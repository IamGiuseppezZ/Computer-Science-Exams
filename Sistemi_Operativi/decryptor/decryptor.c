#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <ctype.h>
#include "mac_sem.h"

#define MAX_BUFFER 256

// Strutture Dati 

typedef struct {
    int num_keys;
    char** argv;
} Par;

// Struttura globale per la comunicazione Request-Reply
typedef struct {
    char frase_cifrata[MAX_BUFFER];
    char frase_decifrata[MAX_BUFFER];
    int cifrario_target;
    
    int flag_finished; // Poison Pill
    
    mac_sem_t cypertext_sem;    // Semaforo su cui attende il Cypertext (Risposta pronta)
    mac_sem_t* sem_decrypt;     // Array di semafori (uno per ogni Decryptor)
    
    pthread_mutex_t mutex;      // Mutex generale (se necessario)
} Struct_Shared;

// Struttura privata passata a ogni singolo Decryptor
typedef struct {
    Struct_Shared* shared;
    int id_thread;
    char chiave[MAX_BUFFER];
} Shared_Thread;

//  Funzioni di Supporto 

// Conta le chiavi (righe) presenti nel file dei cifrari
int calc_cifrario(const char* filepath) {
    FILE* fd = fopen(filepath, "r");
    if (fd == NULL) {
        perror("[-] Errore apertura file cifrario");
        exit(EXIT_FAILURE);
    }
    int counter = 0;
    char buffer[MAX_BUFFER];
    while (fgets(buffer, MAX_BUFFER, fd) != NULL) {
        counter++;
    }
    fclose(fd);
    return counter;
}

// Carica le chiavi dal file a un array di stringhe allocato dinamicamente
void carica_cifrari(const char* filepath, char** array_cifrari) {
    FILE* fd = fopen(filepath, "r");
    if (fd == NULL) {
        perror("[-] Errore apertura file cifrario");
        exit(EXIT_FAILURE);
    }
    char buffer[MAX_BUFFER];
    int counter = 0;
    while (fgets(buffer, MAX_BUFFER, fd) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0'; // Rimuove il newline
        strcpy(array_cifrari[counter], buffer);
        counter++;
    }
    fclose(fd);
}

// Cerca a quale lettera dell'alfabeto standard corrisponde il carattere cifrato
char cerca_pos(char letter, const char* chiave) {
    // L'alfabeto standard è implicito: a=0, b=1, c=2...
    // La chiave contiene la mappatura: es. "qwertyuiop..." 
    // Se cerchiamo 'q', deve restituire 'a'
    
    for (int i = 0; i < 26; ++i) {
        if (tolower(letter) == tolower(chiave[i])) {
            // Se la lettera originale era maiuscola, manteniamo il case
            if (isupper(letter)) {
                return toupper('a' + i);
            } else {
                return 'a' + i;
            }
        }
    }
    return letter; // Se non è una lettera (es. spazio, punteggiatura), ritorna se stesso
}

// Traduce l'intera frase
void decifra_frase(const char* chiave, const char* frase_cifrata, char* frase_decifrata) {
    int i;
    for (i = 0; i < strlen(frase_cifrata); ++i) {
        if (isalpha(frase_cifrata[i])) {
            frase_decifrata[i] = cerca_pos(frase_cifrata[i], chiave);
        } else {
            frase_decifrata[i] = frase_cifrata[i];
        }
    }
    frase_decifrata[i] = '\0'; // Terminatore di stringa
}

//  Thread Functions 

// THREAD WORKER:
void* decryptor_func(void* args) {
    Shared_Thread* thread_info = (Shared_Thread*) args;
    Struct_Shared* shared = thread_info->shared;
    int id = thread_info->id_thread;

    while (1) {
        // Attende che il Cypertext gli assegni un lavoro
        mac_sem_wait(&shared->sem_decrypt[id]);

        // Controllo della Poison Pill (Terminazione)
        if (shared->flag_finished == 1) {
            break;
        }

        // Il thread lavora. Non serve il mutex qui perché il Cypertext 
        // è bloccato ad aspettare e nessuno scriverà su frase_cifrata
        decifra_frase(thread_info->chiave, shared->frase_cifrata, shared->frase_decifrata);

        // Notifica al Cypertext che il lavoro è completato
        mac_sem_post(&shared->cypertext_sem);
    }
    
    free(args); // Prevenzione Memory Leak
    return NULL;
}

// THREAD DELEGATOR: Legge il file e smista il lavoro
void* cypertext_func(void* args) {
    Struct_Shared* shared = (Struct_Shared*) args;
    
    // Si assume che il file dei messaggi sia in argv[2]
    FILE* fd = fopen("frasi_cifrate.txt", "r"); // Usa i nomi dei file definiti dal docente
    if (fd == NULL) {
        perror("[-] Errore apertura file delle frasi");
        exit(EXIT_FAILURE);
    }

    char buffer[MAX_BUFFER];
    while (fgets(buffer, sizeof(buffer), fd) != NULL) {
        
        // Parsing della riga nel formato "ID_Chiave:Frase Segreta"
        char* id_str = strtok(buffer, ":");
        char* line = strtok(NULL, "\n");
        
        if (id_str && line) {
            int id_key = atoi(id_str) - 1; // Mappiamo da 1-N a 0-(N-1) per gli array

            strcpy(shared->frase_cifrata, line);
            shared->cifrario_target = id_key;

            printf("[CYPERTEXT] Inviata frase: '%s' al Decifratore [%d]\n", shared->frase_cifrata, id_key + 1);

            // 1. Sveglia il Decifratore specifico
            mac_sem_post(&shared->sem_decrypt[id_key]);

            // 2. Si addormenta in attesa della risposta
            mac_sem_wait(&shared->cypertext_sem);

            // 3. Stampa il risultato
            printf("[CYPERTEXT] Risultato: '%s'\n\n", shared->frase_decifrata);
        }
    }
    fclose(fd);

    // Gestione Terminazione Globale (Poison Pill)
    shared->flag_finished = 1;
    
    // Sveglia tutti i thread decifratori così possono leggere il flag e terminare
    for (int i = 0; i < shared->cifrario_target + 10; ++i) { // Usa num_keys reale nel main
        // Essendo nel thread delegator non abbiamo par.num_keys direttamente, 
        // ma lo gestiremo dal main. Qui ci affidiamo al loop nel main per la join.
    }
    
    return NULL;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "Uso: %s <file_cifrari.txt> <file_frasi.txt>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int num_cifrari = calc_cifrario(argv[1]);
    
    // Allocazione dinamica matrice stringhe per i cifrari
    char** array_cifrari = malloc(num_cifrari * sizeof(char*));
    for(int i = 0; i < num_cifrari; i++) {
        array_cifrari[i] = malloc(MAX_BUFFER * sizeof(char));
    }
    carica_cifrari(argv[1], array_cifrari);

    // Inizializzazione Struttura Condivisa
    Struct_Shared* shared = (Struct_Shared*) malloc(sizeof(Struct_Shared));
    shared->sem_decrypt = (mac_sem_t*) malloc(sizeof(mac_sem_t) * num_cifrari);
    shared->flag_finished = 0;

    // Sincronizzazione: Cypertext parte bloccato (aspetterà i decifratori), 
    // Decryptor partono bloccati (aspettano ordini)
    mac_sem_init(&shared->cypertext_sem, 0, 0); 
    for(int i = 0; i < num_cifrari; ++i){
        mac_sem_init(&shared->sem_decrypt[i], 0, 0);
    }
    pthread_mutex_init(&shared->mutex, NULL);

    // Creazione Thread
    pthread_t cypertext;
    pthread_t decryptor[num_cifrari];

    for(int i = 0; i < num_cifrari; ++i){
        Shared_Thread* shared_thread = (Shared_Thread*) malloc(sizeof(Shared_Thread));
        shared_thread->shared = shared;
        shared_thread->id_thread = i; // Eliminata la malloc inutile dell'ID
        strcpy(shared_thread->chiave, array_cifrari[i]);
        
        if(pthread_create(&decryptor[i], NULL, decryptor_func, shared_thread) != 0){
            perror("[-] Errore creazione thread decryptor");
            exit(EXIT_FAILURE);
        }
    }
    
    if (pthread_create(&cypertext, NULL, cypertext_func, shared) != 0){
        perror("[-] Errore creazione thread cypertext");
        exit(EXIT_FAILURE);
    }

    // Attesa Terminazione (Join)
    pthread_join(cypertext, NULL);
    
    // Poiché cypertext è terminato, svegliamo i decryptor per farli uscire
    for(int i = 0; i < num_cifrari; ++i){
        mac_sem_post(&shared->sem_decrypt[i]);
        pthread_join(decryptor[i], NULL);
    }

    // Pulizia e Distruzione
    for(int i = 0; i < num_cifrari; ++i){
        mac_sem_destroy(&shared->sem_decrypt[i]);
        free(array_cifrari[i]);
    }
    free(array_cifrari);
    mac_sem_destroy(&shared->cypertext_sem);
    pthread_mutex_destroy(&shared->mutex);
    free(shared->sem_decrypt);
    free(shared);

    return EXIT_SUCCESS;
}
