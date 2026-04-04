#include "mac_sem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#define BUFFER_SIZE 3
#define POISON_PILL "EOF_TERMINATE"

// Struttura che viaggia da DIR a STAT (Contiene solo il nome per ora)
typedef struct {
    char path_name[255];
} File_To_Stat;

// Struttura che viaggia da STAT a MAIN (Contiene nome e dimensione)
typedef struct {
    char path_name[255];
    long long int size_file;
} File_To_Main;

typedef struct {
    // Buffer Stadio 1 (DIR -> STAT)
    File_To_Stat file_buffer[BUFFER_SIZE];
    int in_stat;
    int out_stat;
    
    // Semafori Stadio 1
    mac_sem_t buffer_empty_stat_sem;
    mac_sem_t buffer_full_stat_sem;
    pthread_mutex_t mutex_buffer_stat;
    
    // Contatore per capire quando tutti i thread DIR hanno finito
    int active_dir_threads;
    pthread_mutex_t mutex_dir_count;

    // Buffer Stadio 2 (STAT -> MAIN) gestito con Variabile di Condizione
    File_To_Main file_main;
    int file_ready; // Flag booleano: 1 se STAT ha prodotto, 0 se MAIN ha consumato
    int flag_uscita_main; // Segnala al MAIN che è finita
    
    pthread_cond_t cond_main;
    pthread_cond_t cond_stat; // Aggiunta cond per far aspettare STAT
    pthread_mutex_t mutex_main;
} Struct_Shared;

typedef struct {
    Struct_Shared* shared;
    char dir_path[255];
    int id_thread;
} Shared_Thread;

// 1. PRODUTTORE: Cerca i file e li manda a STAT
void* dir_func(void* args){
    Shared_Thread* shared_thread = (Shared_Thread*) args;
    Struct_Shared* shared = shared_thread->shared;
    
    DIR* directory;
    struct dirent* entry;
    
    if ((directory = opendir(shared_thread->dir_path)) == NULL){
        perror("[-] Errore apertura directory");
        free(args);
        return NULL;
    }
    
    while((entry = readdir(directory)) != NULL){
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0){
            if(entry->d_type == DT_REG) { // Assicuriamoci sia un file regolare
                
                mac_sem_wait(&shared->buffer_empty_stat_sem);
                pthread_mutex_lock(&shared->mutex_buffer_stat);
                
                char complete_path[255];
                snprintf(complete_path, sizeof(complete_path), "%s/%s", shared_thread->dir_path, entry->d_name);
                
                strcpy(shared->file_buffer[shared->in_stat].path_name, complete_path);
                shared->in_stat = (shared->in_stat + 1) % BUFFER_SIZE;
                
                printf("[DIR-%d] Trovato file: '%s'\n", shared_thread->id_thread, complete_path);
                
                pthread_mutex_unlock(&shared->mutex_buffer_stat);
                mac_sem_post(&shared->buffer_full_stat_sem);
            }
        }
    }
    closedir(directory);

    // Gestione Terminazione
    pthread_mutex_lock(&shared->mutex_dir_count);
    shared->active_dir_threads--;
    if (shared->active_dir_threads == 0) {
        // Ultimo thread DIR: invia la Poison Pill a STAT
        mac_sem_wait(&shared->buffer_empty_stat_sem);
        pthread_mutex_lock(&shared->mutex_buffer_stat);
        strcpy(shared->file_buffer[shared->in_stat].path_name, POISON_PILL);
        shared->in_stat = (shared->in_stat + 1) % BUFFER_SIZE;
        pthread_mutex_unlock(&shared->mutex_buffer_stat);
        mac_sem_post(&shared->buffer_full_stat_sem);
    }
    pthread_mutex_unlock(&shared->mutex_dir_count);

    free(args); // Previene memory leak
    return NULL;
}

// 2. CONSUMATORE/PRODUTTORE: Calcola stat() e sveglia MAIN con Condition Variables
void* stat_func(void* args){
    Struct_Shared* shared = (Struct_Shared*) args;
    struct stat file_stat;

    while(1){
        // Preleva dal buffer di DIR
        mac_sem_wait(&shared->buffer_full_stat_sem);
        pthread_mutex_lock(&shared->mutex_buffer_stat);
        
        File_To_Stat file_info = shared->file_buffer[shared->out_stat];
        shared->out_stat = (shared->out_stat + 1) % BUFFER_SIZE;
        
        pthread_mutex_unlock(&shared->mutex_buffer_stat);
        mac_sem_post(&shared->buffer_empty_stat_sem);

        // Controllo Terminazione
        if (strcmp(file_info.path_name, POISON_PILL) == 0) {
            // Segnala al MAIN di uscire
            pthread_mutex_lock(&shared->mutex_main);
            shared->flag_uscita_main = 1;
            shared->file_ready = 1; // Faccio svegliare MAIN dalla wait
            pthread_cond_signal(&shared->cond_main);
            pthread_mutex_unlock(&shared->mutex_main);
            break;
        }

        // Calcola la dimensione
        long long int size = 0;
        if (stat(file_info.path_name, &file_stat) == 0) {
            size = file_stat.st_size;
            printf("[STAT] Analizzato '%s' -> [%lld bytes]\n", file_info.path_name, size);
        } else {
            perror("[-] Errore in stat()");
        }

        // Invio al MAIN (Sincronizzazione stile Ping-Pong)
        pthread_mutex_lock(&shared->mutex_main);
        
        // Aspetta che MAIN consumi il file precedente
        while (shared->file_ready == 1) {
            pthread_cond_wait(&shared->cond_stat, &shared->mutex_main);
        }

        // Inserisce il nuovo file
        strcpy(shared->file_main.path_name, file_info.path_name);
        shared->file_main.size_file = size;
        shared->file_ready = 1;
        
        // Sveglia MAIN
        pthread_cond_signal(&shared->cond_main);
        
        pthread_mutex_unlock(&shared->mutex_main);
    }
    return NULL;
}

// 3. CONSUMATORE FINALE: Somma le dimensioni
void* main_func(void* args){
    Struct_Shared* shared = (Struct_Shared*) args;
    long long int parziale = 0;

    pthread_mutex_lock(&shared->mutex_main); // Acquisisce il mutex fuori dal ciclo
    
    while(1){
        // Aspetta finché STAT non produce un file
        while(!shared->file_ready){
            pthread_cond_wait(&shared->cond_main, &shared->mutex_main);
        }

        // Se STAT ha segnalato l'uscita
        if (shared->flag_uscita_main == 1){
            break;
        }

        // Consuma il file
        parziale += shared->file_main.size_file;
        printf("[MAIN] Aggiunto '%s' - Somma Parziale: [%lld bytes]\n", shared->file_main.path_name, parziale);
        
        // Segnala a STAT che lo slot è libero
        shared->file_ready = 0;
        pthread_cond_signal(&shared->cond_stat);
    }
    pthread_mutex_unlock(&shared->mutex_main); // Rilascia alla fine

    printf("\n=========================================\n");
    printf("        DIMENSIONE TOTALE: [%lld bytes]\n", parziale);
    printf("=========================================\n");
    
    return NULL;
}

int main(int argc, char** argv){
    if(argc < 2){
        fprintf(stderr, "Uso: %s <dir1> <dir2> ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_directories = argc - 1;

    Struct_Shared* shared = (Struct_Shared*) malloc(sizeof(Struct_Shared));
    if(!shared){
        perror("Errore malloc");
        exit(EXIT_FAILURE);
    }

    // Inizializzazione Struttura
    shared->in_stat = 0;
    shared->out_stat = 0;
    shared->active_dir_threads = num_directories;
    shared->file_ready = 0;
    shared->flag_uscita_main = 0;

    // Inizializzazione Sincronizzazione
    pthread_mutex_init(&shared->mutex_buffer_stat, NULL);
    pthread_mutex_init(&shared->mutex_main, NULL);
    pthread_mutex_init(&shared->mutex_dir_count, NULL);
    
    mac_sem_init(&shared->buffer_empty_stat_sem, 0, BUFFER_SIZE);
    mac_sem_init(&shared->buffer_full_stat_sem, 0, 0);
    
    pthread_cond_init(&shared->cond_main, NULL);
    pthread_cond_init(&shared->cond_stat, NULL); // Necessario per il ping-pong

    // Creazione Thread
    pthread_t stat_thread, main_thread;
    pthread_t dir_thread[num_directories];

    pthread_create(&main_thread, NULL, main_func, shared);
    pthread_create(&stat_thread, NULL, stat_func, shared);

    for(int i = 0; i < num_directories; ++i){
        Shared_Thread* shared_thread = (Shared_Thread*) malloc(sizeof(Shared_Thread));
        shared_thread->shared = shared;
        shared_thread->id_thread = i;
        strcpy(shared_thread->dir_path, argv[i + 1]);
        
        pthread_create(&dir_thread[i], NULL, dir_func, shared_thread);
    }

    // Attesa Thread
    for(int i = 0; i < num_directories; ++i){
        pthread_join(dir_thread[i], NULL);
    }
    pthread_join(stat_thread, NULL);
    pthread_join(main_thread, NULL);

    // Pulizia
    pthread_mutex_destroy(&shared->mutex_buffer_stat);
    pthread_mutex_destroy(&shared->mutex_main);
    pthread_mutex_destroy(&shared->mutex_dir_count);
    mac_sem_destroy(&shared->buffer_empty_stat_sem);
    mac_sem_destroy(&shared->buffer_full_stat_sem);
    pthread_cond_destroy(&shared->cond_main);
    pthread_cond_destroy(&shared->cond_stat);
    free(shared);

    return EXIT_SUCCESS;
}