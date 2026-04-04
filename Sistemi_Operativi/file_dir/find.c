#include "mac_sem.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>

#define MAX_BUFFER 3

struct struct_file
{
    char dir[255];
    char name_file[255];
    int occorrenze;
    int read;
};
typedef struct struct_file File_Type;

struct struct_shared
{
    File_Type file[MAX_BUFFER];
    pthread_mutex_t mutex;
    char word[100];
    int in;
    int out;
    int in_verify;
    int out_verify;
    int flag_file;
    int flag_search;
    int flag_main;
    File_Type file_main[MAX_BUFFER];
    mac_sem_t sem_empty;
    mac_sem_t sem_full;
    mac_sem_t sem_main;
    mac_sem_t verify_empty_sem;
    mac_sem_t verify_full_sem;
};
typedef struct struct_shared Struct_Shared;

struct struct_par
{
    int n;
    char **argv;
};
typedef struct struct_par Par;

struct struct_shared_thread
{
    Par par;
    Struct_Shared *shared;
    int id_thread;
};
typedef struct struct_shared_thread Struct_Shared_Thread;

// FUNZIONI THREAD
void *dir_function(void *args)
{
    Struct_Shared_Thread *shared = (Struct_Shared_Thread *)args;
    DIR *directory;
    struct dirent *entry;
    directory = opendir(shared->par.argv[shared->id_thread + 2]);
    printf("[DIR-%d] Analizzo dir: '%s/'\n", shared->id_thread, shared->par.argv[shared->id_thread + 2]);
    if (directory == NULL)
    {
        perror("Error opening directory.");
        exit(EXIT_FAILURE);
    }
    while ((entry = readdir(directory)) != NULL)
    {
        if (entry->d_type == DT_REG)
        {
            int sem_value;
            // Diminuisco uno spazio nel buffer con la down del sem_empty
            mac_sem_wait(&shared->shared->sem_empty);
            pthread_mutex_lock(&shared->shared->mutex);
            File_Type file_type;
            file_type.read = 0;
            strcpy(file_type.dir, shared->par.argv[shared->id_thread + 2]);
            strcpy(file_type.name_file, entry->d_name);
            printf("[DIR-%d] trovato il file: '%s/%s'\n", shared->id_thread, shared->par.argv[shared->id_thread + 2], entry->d_name);
            shared->shared->file[shared->shared->in] = file_type;
            shared->shared->in = (shared->shared->in + 1) % MAX_BUFFER;
            pthread_mutex_unlock(&shared->shared->mutex);
            // Dico che c'è un elemento in più
            mac_sem_post(&shared->shared->sem_full);
        }
    }
    shared->shared->flag_file--;
    printf("[DIR] FINISHED\n");
    return NULL;
}

int num_occorrenze(char *dir, char *name, char *word)
{
    int counter = 0;
    char original_dir[100];

    // Salva la directory corrente
    if (getcwd(original_dir, sizeof(original_dir)) == NULL)
    {
        perror("Error getting current directory");
        exit(EXIT_FAILURE);
    }
    if (chdir(dir) == 0)
    {
        FILE *fd = fopen(name, "r");
        if (fd == NULL)
        {
            perror("Error opening file.");
            exit(EXIT_FAILURE);
        }
        char buffer[255];
        while (fgets(buffer, 255, fd) != NULL)
        {
            for (int i = 0; buffer[i] != '\0'; i++)
            {
                buffer[i] = tolower(buffer[i]); // Trasforma ogni carattere in minuscolo
            }
            char* test = buffer;
            char *ptr;
            while ((ptr = strstr(test, word)) != NULL)
            {
                counter++;
                test = ptr +strlen(word);
            }
        }
        fclose(fd);
    }
    else
    {
        perror("Error changing directory.\n");
        exit(EXIT_FAILURE);
    }
    if (chdir(original_dir) != 0)
    {
        perror("Error returning to original directory");
        exit(EXIT_FAILURE);
    }
    return counter;
}

void *search_function(void *args)
{
    Struct_Shared *struct_shared = (Struct_Shared *)args;
    while (1)
    {
        mac_sem_wait(&struct_shared->sem_full);
        mac_sem_wait(&struct_shared->verify_empty_sem);
        pthread_mutex_lock(&struct_shared->mutex);
        printf("[SEARCH] Analizzo file: %s/%s\n", struct_shared->file[struct_shared->out].dir, struct_shared->file[struct_shared->out].name_file);
        int num = num_occorrenze(struct_shared->file[struct_shared->out].dir, struct_shared->file[struct_shared->out].name_file, struct_shared->word);
        printf("[SEARCH] Il file %s contiene [%d] occorrenze.\n", struct_shared->file[struct_shared->out].name_file, num);
        File_Type file_main;
        strcpy(file_main.dir, struct_shared->file[struct_shared->out].dir);
        strcpy(file_main.name_file, struct_shared->file[struct_shared->out].name_file);
        file_main.occorrenze = num;
        struct_shared->file_main[struct_shared->in_verify] = file_main;
        struct_shared -> in_verify = (struct_shared->in_verify + 1) % MAX_BUFFER;
        struct_shared->out = (struct_shared->out + 1) % MAX_BUFFER;
        if (struct_shared->flag_file == 0 && struct_shared->out == 0){
            struct_shared->flag_search = 0;
            pthread_mutex_unlock(&struct_shared->mutex);
            mac_sem_post(&struct_shared->verify_full_sem);
            printf("[SEARCH] FINISHED.\n");
            break;
        }
        printf("[SEARCH] %d\n", struct_shared->out);
        pthread_mutex_unlock(&struct_shared->mutex);
        mac_sem_post(&struct_shared->verify_full_sem);
        mac_sem_post(&struct_shared->sem_empty);
    }

    return NULL;
}

void *main_function(void *args)
{
    Struct_Shared* shared = (Struct_Shared*) args;
    int occorrenze = 0;
    while(1){
        mac_sem_wait(&shared->verify_full_sem);
        pthread_mutex_lock(&shared->mutex);
        printf("[MAIN] Aggiungo il file: '%s/%s' con occorrenze: %d\n", shared->file_main[shared->out_verify].dir, shared->file_main[shared->out_verify].name_file, shared->file_main[shared->out_verify].occorrenze);
        occorrenze += shared->file_main[shared->out_verify].occorrenze;
        printf("[MAIN] Totale Parziale: %d\n", occorrenze);
        shared->out_verify = (shared->out_verify + 1) % MAX_BUFFER;
        if (shared->flag_file == 0 && shared->flag_search == 0){
            break;
        }
        pthread_mutex_unlock(&shared->mutex);
        mac_sem_post(&shared->verify_empty_sem);
    }
    while(shared->out_verify != 0){
        printf("[MAIN] Aggiungo il file: '%s/%s' con occorrenze: %d\n", shared->file_main[shared->out_verify].dir, shared->file_main[shared->out_verify].name_file, shared->file_main[shared->out_verify].occorrenze);
        occorrenze += shared->file_main[shared->out_verify].occorrenze;
        printf("[MAIN] Totale Parziale: %d\n", occorrenze);
        printf("File rimanenti: %d\n", shared->out_verify);
        shared->out_verify = (shared->out_verify + 1) % MAX_BUFFER;   
    }
    printf("[MAIN] Occorrenze finali: [%d]\n", occorrenze);
    
    

    return NULL;
}

int main(int argc, char **argv)
{
    srand(time(NULL));
    char *word_to_find = (char *)malloc(sizeof(char) * strlen(argv[1] + 1));
    strcpy(word_to_find, argv[1]);
    Par par;
    par.n = argc - 2;
    par.argv = argv;
    Struct_Shared *struct_shared = (Struct_Shared *)malloc(sizeof(Struct_Shared));
    strcpy(struct_shared->word, word_to_find);
    struct_shared->in = 0;
    struct_shared->out = 0;
    struct_shared->in_verify = 0;
    struct_shared->out_verify = 0;
    struct_shared->flag_file = par.n;
    struct_shared->flag_main = 1;
    struct_shared->flag_search = 1;

    // CREO MUTEX E SEM
    mac_sem_init(&struct_shared->sem_full, 0, 0);
    mac_sem_init(&struct_shared->sem_empty, 0, MAX_BUFFER);
    mac_sem_init(&struct_shared->verify_full_sem, 0, 0);
    mac_sem_init(&struct_shared->verify_empty_sem, 0, MAX_BUFFER);
    pthread_mutex_init(&struct_shared->mutex, NULL);
    // creo thread
    pthread_t dir_thread[par.n];
    pthread_t search_thread;
    pthread_t main_thread;
    pthread_create(&main_thread, NULL, main_function, (Struct_Shared *)struct_shared);
    pthread_create(&search_thread, NULL, search_function, (Struct_Shared *)struct_shared);
    for (size_t i = 0; i < par.n; ++i)
    {
        Struct_Shared_Thread *shared_thread = (Struct_Shared_Thread *)malloc(sizeof(Struct_Shared_Thread));
        int *id = (int *)malloc(sizeof(int));
        *id = i;
        shared_thread->shared = struct_shared;
        shared_thread->id_thread = *id;
        shared_thread->par = par;
        pthread_create(&dir_thread[i], NULL, dir_function, (Struct_Shared_Thread *)shared_thread);
    }
    pthread_join(search_thread, NULL);
    pthread_join(main_thread, NULL);
    for (size_t i = 0; i < par.n; ++i)
    {
        pthread_join(dir_thread[i], NULL);
    }
}

// n DIR-I thread, che sono il numero di directory, ogni thread aggiunge il nome del file nella struct
// SEARCH thread che cerca le occorrenze scorrendo ogni file contenuto nell'array condiviso.
/*
Il numero di occorrenze trovate da search e il file, verranno passati al thread MAIN che avrà
il compito di occuparsi di tenere da parte il conteggio delle occorrenze, in base al numero
di occorrenze che arrivano lui deve stampare il parziale
*/