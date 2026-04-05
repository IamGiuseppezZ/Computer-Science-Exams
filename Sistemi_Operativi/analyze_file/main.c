#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <semaphore.h>

#define MAX_BUFFER 2
#define POISON_PILL "EOF_TERMINATE"
#define MAX_LINE_LEN 2048

typedef struct {
    char filepath[FILENAME_MAX];
} Task_File;

typedef struct {
    char filepath[FILENAME_MAX];
    int righe;
    int parole;
    int lettere;
} Task_Result;

// Struttura dati condivisa per tutta la pipeline
typedef struct {
    // BUFFER 1: Analyzer -> Calculator
    Task_File buffer1[MAX_BUFFER];
    int in1;
    int out1;
    sem_t empty1;
    sem_t full1;
    pthread_mutex_t mutex1;

    // BUFFER 2: Calculator -> Aggregator
    Task_Result buffer2[MAX_BUFFER];
    int in2;
    int out2;
    sem_t empty2;
    sem_t full2;
    pthread_mutex_t mutex2;

    // Gestione terminazione
    int active_analyzers;
    pthread_mutex_t mutex_analyzers;
} Pipeline;

// Struttura per passare gli argomenti agli Analyzer
typedef struct {
    Pipeline *shared;
    int id_thread;
    char dir_to_read[FILENAME_MAX];
} Analyzer_Args;


// THREAD STADIO 1: Analyzer (Produttore B1)
void* analyzer_func(void* args) {
    Analyzer_Args* info = (Analyzer_Args*) args;
    Pipeline* shared = info->shared;
    
    DIR* directory;
    struct dirent* entry;
    
    if ((directory = opendir(info->dir_to_read)) != NULL) {
        while ((entry = readdir(directory)) != NULL) {
            if (entry->d_type == DT_REG) {
                char fullpath[FILENAME_MAX];
                snprintf(fullpath, sizeof(fullpath), "%s/%s", info->dir_to_read, entry->d_name);
                
                // Push nel Buffer 1
                sem_wait(&shared->empty1);
                pthread_mutex_lock(&shared->mutex1);
                
                strcpy(shared->buffer1[shared->in1].filepath, fullpath);
                shared->in1 = (shared->in1 + 1) % MAX_BUFFER;
                printf("[Analyzer-%d] Pusho file: '[%s]'\n", info->id_thread, fullpath);
                
                pthread_mutex_unlock(&shared->mutex1);
                sem_post(&shared->full1);
            }
        }
        closedir(directory);
    } else {
        perror("[-] Errore apertura directory");
    }

    // Gestione terminazione: l'ultimo Analyzer inserisce la Poison Pill
    pthread_mutex_lock(&shared->mutex_analyzers);
    shared->active_analyzers--;
    if (shared->active_analyzers == 0) {
        sem_wait(&shared->empty1);
        pthread_mutex_lock(&shared->mutex1);
        
        strcpy(shared->buffer1[shared->in1].filepath, POISON_PILL);
        shared->in1 = (shared->in1 + 1) % MAX_BUFFER;
        
        pthread_mutex_unlock(&shared->mutex1);
        sem_post(&shared->full1);
    }
    pthread_mutex_unlock(&shared->mutex_analyzers);

    free(info);
    return NULL;
}


// THREAD STADIO 2: Calculator (Consumatore B1 / Produttore B2)
void* calculator_func(void* args) {
    Pipeline* shared = (Pipeline*) args;

    while (1) {
        // Estrazione dal Buffer 1
        sem_wait(&shared->full1);
        pthread_mutex_lock(&shared->mutex1);
        
        Task_File task = shared->buffer1[shared->out1];
        shared->out1 = (shared->out1 + 1) % MAX_BUFFER;
        
        pthread_mutex_unlock(&shared->mutex1);
        sem_post(&shared->empty1);

        // Controllo Poison Pill
        if (strcmp(task.filepath, POISON_PILL) == 0) {
            // Propago la pillola all'Aggregator
            sem_wait(&shared->empty2);
            pthread_mutex_lock(&shared->mutex2);
            strcpy(shared->buffer2[shared->in2].filepath, POISON_PILL);
            shared->in2 = (shared->in2 + 1) % MAX_BUFFER;
            pthread_mutex_unlock(&shared->mutex2);
            sem_post(&shared->full2);
            break;
        }

        // Elaborazione del file
        FILE* file = fopen(task.filepath, "r");
        if (!file) continue;

        int c_righe = 0, c_parole = 0, c_lettere = 0;
        char line[MAX_LINE_LEN];

        while (fgets(line, sizeof(line), file)) {
            c_righe++;
            
            // Conteggio lettere
            for (int i = 0; line[i] != '\0'; i++) {
                if (isalpha(line[i])) c_lettere++;
            }

            // Conteggio parole con strtok_r
            char* saveptr;
            char* token = strtok_r(line, " \t\n\r", &saveptr);
            while (token != NULL) {
                c_parole++;
                token = strtok_r(NULL, " \t\n\r", &saveptr);
            }
        }
        fclose(file);
        
        printf("[CALCULATE] File: '[%s]' Analizzato.\n", task.filepath);

        // Push dei risultati nel Buffer 2
        sem_wait(&shared->empty2);
        pthread_mutex_lock(&shared->mutex2);
        
        strcpy(shared->buffer2[shared->in2].filepath, task.filepath);
        shared->buffer2[shared->in2].righe = c_righe;
        shared->buffer2[shared->in2].parole = c_parole;
        shared->buffer2[shared->in2].lettere = c_lettere;
        shared->in2 = (shared->in2 + 1) % MAX_BUFFER;
        
        pthread_mutex_unlock(&shared->mutex2);
        sem_post(&shared->full2);
    }

    return NULL;
}


// THREAD STADIO 3: Aggregator (Consumatore B2)
void* aggregator_func(void* args) {
    Pipeline* shared = (Pipeline*) args;
    
    int tot_righe = 0;
    int tot_parole = 0;
    int tot_lettere = 0;

    while (1) {
        // Estrazione dal Buffer 2
        sem_wait(&shared->full2);
        pthread_mutex_lock(&shared->mutex2);
        
        Task_Result res = shared->buffer2[shared->out2];
        shared->out2 = (shared->out2 + 1) % MAX_BUFFER;
        
        pthread_mutex_unlock(&shared->mutex2);
        sem_post(&shared->empty2);

        // Controllo terminazione
        if (strcmp(res.filepath, POISON_PILL) == 0) break;

        printf("[MAIN] Ricevuto '%s' -> Parziali | Righe: %d | Parole: %d | Lettere: %d\n", 
               res.filepath, res.righe, res.parole, res.lettere);

        tot_righe += res.righe;
        tot_parole += res.parole;
        tot_lettere += res.lettere;
    }

    printf("=========================================\n");
    printf("STATISTICHE FINALI GLOBALI:\n");
    printf("Totale Righe   : %d\n", tot_righe);
    printf("Totale Parole  : %d\n", tot_parole);
    printf("Totale Lettere : %d\n", tot_lettere);
    printf("=========================================\n");

    return NULL;
}


// MAIN
int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <dir1> <dir2> ... <dirN>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_analyzers = argc - 1;

    Pipeline* pipeline = (Pipeline*) malloc(sizeof(Pipeline));
    pipeline->in1 = 0; pipeline->out1 = 0;
    pipeline->in2 = 0; pipeline->out2 = 0;
    pipeline->active_analyzers = num_analyzers;

    // Inizializzazione sync
    sem_init(&pipeline->empty1, 0, MAX_BUFFER);
    sem_init(&pipeline->full1, 0, 0);
    pthread_mutex_init(&pipeline->mutex1, NULL);

    sem_init(&pipeline->empty2, 0, MAX_BUFFER);
    sem_init(&pipeline->full2, 0, 0);
    pthread_mutex_init(&pipeline->mutex2, NULL);
    
    pthread_mutex_init(&pipeline->mutex_analyzers, NULL);

    pthread_t calc_t, agg_t;
    pthread_t analyzer_t[num_analyzers];

    // Avvio Aggregator e Calculator
    if (pthread_create(&agg_t, NULL, aggregator_func, pipeline) != 0 ||
        pthread_create(&calc_t, NULL, calculator_func, pipeline) != 0) {
        perror("[-] Errore creazione thread worker");
        exit(EXIT_FAILURE);
    }

    // Avvio N Analyzers
    for (int i = 0; i < num_analyzers; ++i) {
        Analyzer_Args* info = (Analyzer_Args*) malloc(sizeof(Analyzer_Args));
        info->shared = pipeline;
        info->id_thread = i;
        strcpy(info->dir_to_read, argv[i + 1]);
        
        if (pthread_create(&analyzer_t[i], NULL, analyzer_func, info) != 0) {
            perror("[-] Errore creazione thread analyzer");
            exit(EXIT_FAILURE);
        }
    }

    // Attesa terminazione
    for (int i = 0; i < num_analyzers; ++i) {
        pthread_join(analyzer_t[i], NULL);
    }
    pthread_join(calc_t, NULL);
    pthread_join(agg_t, NULL);

    // Pulizia
    sem_destroy(&pipeline->empty1);
    sem_destroy(&pipeline->full1);
    pthread_mutex_destroy(&pipeline->mutex1);
    
    sem_destroy(&pipeline->empty2);
    sem_destroy(&pipeline->full2);
    pthread_mutex_destroy(&pipeline->mutex2);
    
    pthread_mutex_destroy(&pipeline->mutex_analyzers);
    free(pipeline);

    return EXIT_SUCCESS;
}
