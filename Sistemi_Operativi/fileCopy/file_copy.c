#include "lib-misc.h" // Assumendo che contenga macro come exit_with_sys_err
#include "mac_sem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <fcntl.h>

#define MAX_BUFFER 1024
#define SIZE_BUFFER 10
#define POISON_PILL "EOF_TERMINATE"

// Struttura passata dal main
typedef struct {
    int num_reader;
    char destination_dir[FILENAME_MAX];
    char **argv;
} Parametri;

// Dati del singolo blocco di file in transito
typedef struct {
    char buffer_file[MAX_BUFFER];
    char filename[FILENAME_MAX];
    size_t file_size;
    size_t dimensione_blocco;
    long int offset_file;
} File_Chunk;

// Area di memoria condivisa tra Reader e Writer
typedef struct {
    File_Chunk buffer_file[SIZE_BUFFER];
    char destination_dir[FILENAME_MAX];
    
    int in_buffer;
    int out_buffer;
    
    int active_readers; // Contatore per la terminazione
    
    mac_sem_t empty_buffer;
    mac_sem_t full_buffer;
    pthread_mutex_t mutex;
    pthread_mutex_t mutex_readers;
} Stack;

// Struttura specifica per il singolo thread Reader
typedef struct {
    Stack *shared;
    int id_thread;
    char dir_to_read[FILENAME_MAX];
} Struct_Thread;

// ---- INIZIO LISTA CONCATENATA PER IL WRITER ----
// Serve al Writer per tenere traccia dei file mmap() aperti in scrittura
typedef struct list_file {
    struct list_file* next;
    char filename[FILENAME_MAX];
    char* mapped;
    size_t file_size;
    int fd;
} List;

void addList(List** head, const char* filename, char* mapped, int fd, size_t file_size) {
    List* temp = (List*) malloc(sizeof(List));
    if (!temp) return;
    strcpy(temp->filename, filename);
    temp->mapped = mapped;
    temp->fd = fd;
    temp->file_size = file_size;
    temp->next = *head;
    *head = temp;
}

List* give_file(List* head, const char* filename) {
    while (head != NULL) {
        if (strcmp(head->filename, filename) == 0) {
            return head;
        }
        head = head->next;
    }
    return NULL;
}

void cleanList(List* head) {
    List* current = head;
    while (current != NULL) {
        List* next = current->next;
        // Effettua sync, unmap e chiude il file descritpor
        msync(current->mapped, current->file_size, MS_SYNC);
        munmap(current->mapped, current->file_size);
        close(current->fd);
        free(current);
        current = next;
    }
}
// ---- FINE LISTA CONCATENATA ----


// ==========================================
// THREAD PRODUTTORE: Reader
// ==========================================
void* reader_func(void* args) {
    Struct_Thread* thread_info = (Struct_Thread*) args;
    Stack* shared = thread_info->shared;
    
    DIR* directory;
    struct dirent* entry;
    
    if ((directory = opendir(thread_info->dir_to_read)) == NULL) {
        perror("[-] Errore apertura directory");
        // Non usciamo drasticamente per permettere agli altri thread di finire
        goto end_reader; 
    }

    while ((entry = readdir(directory)) != NULL) {
        if (entry->d_type == DT_REG) { // Se è un file regolare
            if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
                
                char fullpath[FILENAME_MAX];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", thread_info->dir_to_read, entry->d_name);
                
                int fd = open(fullpath, O_RDONLY);
                if (fd == -1) continue;

                struct stat buf;
                fstat(fd, &buf);
                size_t size = buf.st_size;

                if (size == 0) { // Salta i file vuoti
                    close(fd);
                    continue;
                }

                char* mapped = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
                if (mapped == MAP_FAILED) {
                    close(fd);
                    continue;
                }

                size_t offlen = 0;
                while (offlen < size) {
                    File_Chunk chunk;
                    size_t to_sum = MAX_BUFFER;
                    if (offlen + to_sum > size) {
                        to_sum = size - offlen;
                    }

                    // Prepara il chunk in locale
                    memcpy(chunk.buffer_file, mapped + offlen, to_sum);
                    strcpy(chunk.filename, entry->d_name);
                    chunk.file_size = size;
                    chunk.offset_file = offlen;
                    chunk.dimensione_blocco = to_sum;

                    // Push nel buffer condiviso
                    mac_sem_wait(&shared->empty_buffer);
                    pthread_mutex_lock(&shared->mutex);
                    
                    shared->buffer_file[shared->in_buffer] = chunk;
                    shared->in_buffer = (shared->in_buffer + 1) % SIZE_BUFFER;
                    
                    pthread_mutex_unlock(&shared->mutex);
                    mac_sem_post(&shared->full_buffer);

                    offlen += to_sum;
                }

                munmap(mapped, size);
                close(fd);
            }
        }
    }
    closedir(directory);

end_reader:
    // Gestione della terminazione con Poison Pill
    pthread_mutex_lock(&shared->mutex_readers);
    shared->active_readers--;
    if (shared->active_readers == 0) {
        mac_sem_wait(&shared->empty_buffer);
        pthread_mutex_lock(&shared->mutex);
        
        strcpy(shared->buffer_file[shared->in_buffer].filename, POISON_PILL);
        shared->in_buffer = (shared->in_buffer + 1) % SIZE_BUFFER;
        
        pthread_mutex_unlock(&shared->mutex);
        mac_sem_post(&shared->full_buffer);
    }
    pthread_mutex_unlock(&shared->mutex_readers);

    free(thread_info); // Previene memory leak
    return NULL;
}


// ==========================================
// THREAD CONSUMATORE: Writer
// ==========================================
void* writer_func(void* args) {
    Stack* shared = (Stack*) args;
    List* head = NULL;

    // Crea la cartella di destinazione se non esiste
    struct stat st = {0};
    if (stat(shared->destination_dir, &st) == -1) {
        if (mkdir(shared->destination_dir, 0700) == -1) {
            perror("[-] Errore creazione cartella di destinazione");
            exit(EXIT_FAILURE);
        }
    }

    while (1) {
        mac_sem_wait(&shared->full_buffer);
        
        pthread_mutex_lock(&shared->mutex);
        File_Chunk chunk = shared->buffer_file[shared->out_buffer];
        shared->out_buffer = (shared->out_buffer + 1) % SIZE_BUFFER;
        pthread_mutex_unlock(&shared->mutex);
        
        mac_sem_post(&shared->empty_buffer);

        // Controllo della Poison Pill (Terminazione)
        if (strcmp(chunk.filename, POISON_PILL) == 0) {
            printf("\n[WRITER] Tutti i file copiati. Pulizia finale e terminazione...\n");
            break;
        }

        List* file_node = give_file(head, chunk.filename);

        // Se il file non è ancora nella lista, lo creiamo e lo mappiamo
        if (file_node == NULL) {
            char fullpath[FILENAME_MAX];
            snprintf(fullpath, FILENAME_MAX, "%s/%s", shared->destination_dir, chunk.filename);
            
            int fd = open(fullpath, O_RDWR | O_CREAT | O_TRUNC, 0600);
            if (fd == -1) {
                perror("[-] Errore apertura file in scrittura");
                continue;
            }

            if (ftruncate(fd, chunk.file_size) == -1) {
                perror("[-] Errore ridimensionamento file");
                close(fd);
                continue;
            }

            char* mapped = mmap(NULL, chunk.file_size, PROT_WRITE | PROT_READ, MAP_SHARED, fd, 0);
            if (mapped == MAP_FAILED) {
                perror("[-] Errore mmap file in scrittura");
                close(fd);
                continue;
            }

            addList(&head, chunk.filename, mapped, fd, chunk.file_size);
            file_node = head; // Aggiorna il puntatore per scriverci subito
            printf("[WRITER] Creato nuovo file in destinazione: '%s'\n", chunk.filename);
        }

        // Copia i dati del blocco direttamente nella memoria mappata del file
        memcpy(file_node->mapped + chunk.offset_file, chunk.buffer_file, chunk.dimensione_blocco);
    }

    // Libera tutte le mmap() aperte per evitare leak di risorse OS
    cleanList(head);
    return NULL;
}


// ==========================================
// MAIN
// ==========================================
int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <dir_origine_1> [dir_origine_N...] <dir_destinazione>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_reader = argc - 2;

    Stack* stack = (Stack*) malloc(sizeof(Stack));
    stack->in_buffer = 0;
    stack->out_buffer = 0;
    stack->active_readers = num_reader;
    strcpy(stack->destination_dir, argv[argc - 1]);
    
    mac_sem_init(&stack->empty_buffer, 0, SIZE_BUFFER);
    mac_sem_init(&stack->full_buffer, 0, 0);
    pthread_mutex_init(&stack->mutex, NULL);
    pthread_mutex_init(&stack->mutex_readers, NULL);

    pthread_t writer_t;
    pthread_t reader_t[num_reader];
    
    if (pthread_create(&writer_t, NULL, writer_func, stack) != 0) {
        perror("[-] Errore Creazione Writer Thread");
        exit(EXIT_FAILURE);
    }
    
    for (int i = 0; i < num_reader; ++i) {
        Struct_Thread* thread_info = (Struct_Thread*) malloc(sizeof(Struct_Thread));
        thread_info->id_thread = i;
        thread_info->shared = stack;
        strcpy(thread_info->dir_to_read, argv[i + 1]);
        
        if (pthread_create(&reader_t[i], NULL, reader_func, thread_info) != 0) {
            perror("[-] Errore Creazione Reader Thread");
            exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < num_reader; ++i) {
        pthread_join(reader_t[i], NULL);
    }
    pthread_join(writer_t, NULL);

    mac_sem_destroy(&stack->empty_buffer);
    mac_sem_destroy(&stack->full_buffer);
    pthread_mutex_destroy(&stack->mutex);
    pthread_mutex_destroy(&stack->mutex_readers);
    free(stack);

    return EXIT_SUCCESS;
}