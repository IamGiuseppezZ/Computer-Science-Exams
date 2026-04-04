#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "mac_sem.h"

#define MAX_BUFFER 5

typedef long long llong;

typedef struct {
    int num_threads;
    char** argv;
} Parameters;

typedef struct {
    llong op1;      // Accumulatore corrente
    llong op2;      // Numero da operare
    char operator;  // Operazione (+, -, *, /)
    int id_thread;  // ID del mittente per sapere chi risvegliare
} Request;

typedef struct {
    Request request[MAX_BUFFER];
    int in;
    int out;
    
    llong *response;             // Array delle risposte (1 per thread)
    int thread_finished;         // Contatore dei thread che hanno finito
    long checks_completed;       // Contatore delle operazioni svolte
    int num_thread;              // Numero totale di thread file
    
    mac_sem_t full_buffer;       // Notifica al calcolatore (slot pieni)
    mac_sem_t empty_buffer;      // Notifica ai lettori (slot liberi)
    mac_sem_t* sem_files;        // Semafori per svegliare i singoli thread lettori
    
    pthread_mutex_t mutex;       // Protegge indici e variabili globali
} Struct_Shared;

typedef struct {
    int id_thread;
    Parameters par;
    Struct_Shared* shared;
} Struct_Shared_Thread;

// 1. CONSUMATORE / SERVER: Esegue i calcoli
void* calc_func(void* args) {
    Struct_Shared* shared = (Struct_Shared*) args;

    while (1) {
        // Attende che ci sia almeno una richiesta nel buffer
        mac_sem_wait(&shared->full_buffer);

        pthread_mutex_lock(&shared->mutex);
        Request req = shared->request[shared->out];
        shared->out = (shared->out + 1) % MAX_BUFFER;
        pthread_mutex_unlock(&shared->mutex);

        // Segnala che si è liberato uno slot
        mac_sem_post(&shared->empty_buffer);

        // Controllo Terminazione (Poison Pill)
        if (req.id_thread == -1) {
            printf("\n[CALCOLATORE] Ricevuta Poison Pill. Termino.\n");
            printf("[CALCOLATORE] Operazioni totali eseguite: %ld\n", shared->checks_completed);
            break;
        }

        // Calcolo effettivo
        llong res = 0;
        switch (req.operator) {
            case '+': res = req.op1 + req.op2; break;
            case '-': res = req.op1 - req.op2; break;
            case '*': res = req.op1 * req.op2; break;
            case '/': res = (req.op2 != 0) ? (req.op1 / req.op2) : 0; break; // Evita divisione per zero
            default:  res = req.op1; break;
        }

        // Scrive la risposta nell'array condiviso alla posizione dedicata al thread
        shared->response[req.id_thread] = res;
        
        pthread_mutex_lock(&shared->mutex);
        shared->checks_completed++;
        pthread_mutex_unlock(&shared->mutex);

        // Sveglia il thread mittente che era in attesa del risultato
        mac_sem_post(&shared->sem_files[req.id_thread]);
    }
    
    return NULL;
}

// 2. PRODUTTORE / CLIENT: Legge il file e inoltra richieste
void* file_func(void* args) {
    Struct_Shared_Thread* thread_shared = (Struct_Shared_Thread*) args;
    int id = thread_shared->id_thread;
    Struct_Shared* shared = thread_shared->shared;

    // argv[0] è il nome programma, i file partono da argv[1]
    const char* filename = thread_shared->par.argv[id + 1];
    FILE* fd = fopen(filename, "r");
    if (fd == NULL) {
        perror("Errore apertura file");
        free(args);
        return NULL;
    }

    char buffer[256];
    // Salta la prima riga (es. eventuale intestazione)
    fgets(buffer, sizeof(buffer), fd);

    llong accumulator = 0; // Il risultato parte da 0 per ogni file
    char op;
    int num;

    // Legge riga per riga al volo
    while (fgets(buffer, sizeof(buffer), fd) != NULL) {
        if (sscanf(buffer, " %c %d", &op, &num) == 2) {
            
            // FASE 1: Invia Richiesta
            mac_sem_wait(&shared->empty_buffer); // Attende slot libero
            
            pthread_mutex_lock(&shared->mutex);
            shared->request[shared->in].op1 = accumulator;
            shared->request[shared->in].op2 = num;
            shared->request[shared->in].operator = op;
            shared->request[shared->in].id_thread = id;
            shared->in = (shared->in + 1) % MAX_BUFFER;
            pthread_mutex_unlock(&shared->mutex);
            
            mac_sem_post(&shared->full_buffer); // Segnala al Calcolatore

            // FASE 2: Attende Risposta
            mac_sem_wait(&shared->sem_files[id]); 
            
            // FASE 3: Aggiorna il proprio accumulatore
            accumulator = shared->response[id];
            printf("[File-%d] Elaborato: '%c %d' -> Parziale: %lld\n", id, op, num, accumulator);
        }
    }
    
    fclose(fd);
    printf("\n>>> [File-%d] (%s) COMPLETATO. Risultato Finale: %lld <<<\n", id, filename, accumulator);

    // Gestione Terminazione Sicura
    pthread_mutex_lock(&shared->mutex);
    shared->thread_finished++;
    int is_last = (shared->thread_finished == shared->num_thread);
    pthread_mutex_unlock(&shared->mutex);

    // Se sono l'ultimo file a terminare, invio la "Pillola Avvelenata" al calcolatore
    if (is_last) {
        mac_sem_wait(&shared->empty_buffer);
        pthread_mutex_lock(&shared->mutex);
        shared->request[shared->in].id_thread = -1; // -1 indica la terminazione
        shared->in = (shared->in + 1) % MAX_BUFFER;
        pthread_mutex_unlock(&shared->mutex);
        mac_sem_post(&shared->full_buffer);
    }

    free(args); // Rimuove il Memory Leak
    return NULL;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <file1.txt> <file2.txt> ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    Parameters par;
    par.argv = argv;
    par.num_threads = argc - 1;

    // Allocazione Strutture Condivise
    Struct_Shared* shared = (Struct_Shared*) malloc(sizeof(Struct_Shared));
    shared->num_thread = par.num_threads;
    shared->in = 0;
    shared->out = 0;
    shared->thread_finished = 0;
    shared->checks_completed = 0;
    
    // Alloca array dipendenti dal numero di thread
    shared->response = (llong*) malloc(sizeof(llong) * par.num_threads);
    shared->sem_files = (mac_sem_t*) malloc(sizeof(mac_sem_t) * par.num_threads);
    
    // Inizializzazione Sincronizzazione
    pthread_mutex_init(&shared->mutex, NULL);
    mac_sem_init(&shared->full_buffer, 0, 0);               // Elementi nel buffer = 0
    mac_sem_init(&shared->empty_buffer, 0, MAX_BUFFER);     // Spazi liberi = MAX_BUFFER
    
    for(int i = 0; i < par.num_threads; ++i){
        mac_sem_init(&shared->sem_files[i], 0, 0);          // Semafori Client = bloccati
    }

    // Creazione Thread
    pthread_t calc_thread;
    pthread_t file_thread[par.num_threads];
    
    pthread_create(&calc_thread, NULL, calc_func, shared);
    
    for(int i = 0; i < par.num_threads; ++i){
        Struct_Shared_Thread* shared_thread = (Struct_Shared_Thread*) malloc(sizeof(Struct_Shared_Thread));
        shared_thread->shared = shared;
        shared_thread->par = par;
        shared_thread->id_thread = i; // Niente malloc inutile qui!
        
        pthread_create(&file_thread[i], NULL, file_func, shared_thread);
    }

    // Join
    for(int i = 0 ; i < par.num_threads; ++i){
        pthread_join(file_thread[i], NULL);
    }
    pthread_join(calc_thread, NULL);
    
    // Pulizia e Distruzione
    pthread_mutex_destroy(&shared->mutex);
    mac_sem_destroy(&shared->full_buffer);
    mac_sem_destroy(&shared->empty_buffer);
    for(int i = 0; i < par.num_threads; ++i){
        mac_sem_destroy(&shared->sem_files[i]);
    }
    
    free(shared->sem_files);
    free(shared->response);
    free(shared);

    return EXIT_SUCCESS;
}