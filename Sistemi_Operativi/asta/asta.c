#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "mac_sem.h"

#define THREAD 4

typedef struct {
    int value;
    pthread_mutex_t mutex;
    mac_sem_t sem_writer[THREAD]; // Array di semafori, uno per ogni Writer
    mac_sem_t sem_host;           // Semaforo per risvegliare l'Host
} Shared_Data;

typedef struct {
    Shared_Data* shared_data;
    int thread_id;
    unsigned int seed; // Seme personale per rand_r() (Thread-safe)
} Shared_Thread;

// THREAD HOST: Sincronizzatore e Inizializzatore
void* host(void* args) {
    Shared_Data* shared = (Shared_Data*) args;

    // 1. Inizializzazione della risorsa (protetta per sicurezza, anche se è l'unico attivo ora)
    pthread_mutex_lock(&shared->mutex);
    printf("[HOST] Inizializzo il valore condiviso a 1.\n\n");
    shared->value = 1;
    pthread_mutex_unlock(&shared->mutex);

    // 2. Risveglia tutti i Writer (Dà il "Via!")
    for (int i = 0; i < THREAD; ++i) {
        mac_sem_post(&shared->sem_writer[i]);
    }

    // 3. Attende che tutti i Writer abbiano terminato il loro lavoro
    for (int i = 0; i < THREAD; ++i) {
        mac_sem_wait(&shared->sem_host);
    }

    printf("\n[HOST] Tutti i thread Writer hanno completato. Esecuzione terminata.\n");
    return NULL;
}

// THREAD WRITER: Lavoratori
void* writer(void* args) {
    Shared_Thread* thread_info = (Shared_Thread*) args;
    Shared_Data* shared = thread_info->shared_data;
    int id = thread_info->thread_id;
    unsigned int seed = thread_info->seed;

    // 1. Attende il segnale di "Via!" dall'Host
    mac_sem_wait(&shared->sem_writer[id]);

    // 2. Sezione Critica (Lettura e Modifica)
    pthread_mutex_lock(&shared->mutex);
    
    printf("[WRITER-%d] Leggo il valore attuale: %d\n", id, shared->value);
    
    // Generazione numero random thread-safe
    int new_val = 1 + rand_r(&seed) % 100; 
    shared->value = new_val;
    
    printf("[WRITER-%d] Ho modificato il valore in: %d\n", id, shared->value);
    
    pthread_mutex_unlock(&shared->mutex);

    // 3. Notifica all'Host che il lavoro di questo thread è concluso
    mac_sem_post(&shared->sem_host);

    // 4. Prevenzione Memory Leak
    free(thread_info);
    
    return NULL;
}

int main(void) {
    Shared_Data shared;
    pthread_t thread_host;
    pthread_t thread_writer[THREAD];

    // Inizializzazione Mutex e Semafori
    pthread_mutex_init(&shared.mutex, NULL);
    mac_sem_init(&shared.sem_host, 0, 0); // FONDAMENTALE: Inizializzato a 0!
    
    for (int i = 0; i < THREAD; ++i) {
        mac_sem_init(&shared.sem_writer[i], 0, 0);
    }

    // Creazione dei thread Writer
    for (int i = 0; i < THREAD; ++i) {
        Shared_Thread* shared_thread = (Shared_Thread*) malloc(sizeof(Shared_Thread));
        if (!shared_thread) {
            perror("Errore malloc");
            exit(EXIT_FAILURE);
        }
        
        shared_thread->shared_data = &shared;
        shared_thread->thread_id = i;
        // Inizializzo un seed univoco per ogni thread usando il tempo e il suo ID
        shared_thread->seed = (unsigned int)time(NULL) ^ (i * 1337); 

        pthread_create(&thread_writer[i], NULL, writer, shared_thread);
    }

    // Creazione del thread Host
    pthread_create(&thread_host, NULL, host, &shared);

    // Attesa terminazione thread
    for (int i = 0; i < THREAD; ++i) {
        pthread_join(thread_writer[i], NULL);
    }
    pthread_join(thread_host, NULL);

    // Distruzione delle primitive di sincronizzazione
    mac_sem_destroy(&shared.sem_host);
    for (int i = 0; i < THREAD; ++i) {
        mac_sem_destroy(&shared.sem_writer[i]);
    }
    pthread_mutex_destroy(&shared.mutex);

    return EXIT_SUCCESS;
}