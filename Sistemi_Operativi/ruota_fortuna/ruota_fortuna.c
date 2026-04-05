#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "mac_sem.h" 
#include <time.h>
#include <ctype.h>

#define MAX_BUFFER 255

// Parametri da riga di comando
typedef struct {
    int n_players;
    int m_match;
    char file_name[MAX_BUFFER];
} Parametri;

// Dati condivisi tra Dealer e Player
typedef struct {
    char frase_offuscata[MAX_BUFFER];
    char frase_in_chiaro[MAX_BUFFER];
    char lettera_estratta;
    
    int game_over; // Flag di terminazione per i Player
    int* punteggi; // Array dei punteggi dei giocatori
    
    pthread_mutex_t mutex;
    mac_sem_t dealer_sem;
    mac_sem_t *players_sem;
} Struct_Shared;

// Strutture per il passaggio parametri ai thread
typedef struct {
    Struct_Shared* shared;
    int id; // ID del giocatore
} Struct_Shared_Thread;

typedef struct {
    Struct_Shared* shared;
    Parametri* par;
} Struct_Shared_Dealer;

typedef struct {
    char frase[MAX_BUFFER];
} FraseStr;

// FUNZIONI DI SUPPORTO

int get_num_line(FILE* fd) {
    int counter = 0;
    char buffer[MAX_BUFFER];
    while (fgets(buffer, MAX_BUFFER, fd) != NULL) {
        counter++;
    }
    rewind(fd);
    return counter;
}

void fill_array(FraseStr* frasi, int n, FILE* fd) {
    char temp[MAX_BUFFER];
    for(int i = 0; i < n; ++i){
        if (fgets(temp, MAX_BUFFER, fd) == NULL) {
            perror("[-] Errore lettura file");
            exit(EXIT_FAILURE);
        }
        // Rimuove il newline
        temp[strcspn(temp, "\n")] = '\0';
        strcpy(frasi[i].frase, temp);
    }
}

void shuffle_frasi(FraseStr* array, int n) {
    for(int i = 0; i < n; ++i){
        FraseStr temp = array[i];
        int j = rand() % n;
        array[i] = array[j];
        array[j] = temp; 
    }
}

void offusca_frase(const char* chiara, char* offuscata) {
    int i = 0;
    while(chiara[i] != '\0'){
        if (isalpha(chiara[i])) {
            offuscata[i] = '#';
        } else {
            offuscata[i] = chiara[i]; // Mantiene spazi e punteggiatura
        }
        i++;
    }
    offuscata[i] = '\0';
}

void shuffle_lettere(char* lettere, int n) {
    for(int i = 0; i < n; ++i){
        char temp = lettere[i];
        int j = rand() % n;
        lettere[i] = lettere[j];
        lettere[j] = temp;
    }
}

int update_table_and_count(char* offuscata, const char* chiara, char estratta) {
    int occorrenze = 0;
    for(int i = 0; i < strlen(chiara); ++i){
        if (tolower(chiara[i]) == estratta) {
            offuscata[i] = chiara[i];
            occorrenze++;
        }
    }
    return occorrenze;
}

// THREAD DEALER E PLAYER
void* dealer_func(void* args) {
    Struct_Shared_Dealer* thread_info = (Struct_Shared_Dealer*) args;
    Struct_Shared* shared = thread_info->shared;
    Parametri* par = thread_info->par;

    FILE* fd = fopen(par->file_name, "r");
    if (fd == NULL){
        perror("[-] Errore apertura file delle frasi");
        exit(EXIT_FAILURE);
    }

    int num_line = get_num_line(fd);
    if (par->m_match > num_line) {
        printf("[-] Attenzione: Le partite richieste (%d) superano le frasi disponibili (%d). Verranno giocate %d partite.\n", par->m_match, num_line, num_line);
        par->m_match = num_line;
    }

    // Carica e mescola le frasi
    FraseStr* frasi = malloc(num_line * sizeof(FraseStr));
    fill_array(frasi, num_line, fd);
    shuffle_frasi(frasi, num_line);
    fclose(fd);

    printf("==================================================\n");
    printf("[DEALER] Inizia il gioco! (%d Giocatori - %d Partite)\n", par->n_players, par->m_match);
    printf("==================================================\n");

    // Ciclo delle partite
    for (int match = 0; match < par->m_match; ++match) {
        
        char alfabeto[] = "abcdefghijklmnopqrstuvwxyz";
        int num_lettere = 26;
        shuffle_lettere(alfabeto, num_lettere);

        pthread_mutex_lock(&shared->mutex);
        strcpy(shared->frase_in_chiaro, frasi[match].frase);
        offusca_frase(shared->frase_in_chiaro, shared->frase_offuscata);
        pthread_mutex_unlock(&shared->mutex);

        printf("\n>>> PARTITA N. %d <<<\n", match + 1);
        printf("[TABELLONE]: %s\n\n", shared->frase_offuscata);

        // Estrazione lettere
        for (int i = 0; i < num_lettere; ++i) {
            pthread_mutex_lock(&shared->mutex);
            shared->lettera_estratta = alfabeto[i];
            printf("[DEALER] Gira la ruota... Lettera estratta: '%c'\n", shared->lettera_estratta);
            pthread_mutex_unlock(&shared->mutex);

            // Sveglia tutti i Player per fargli "vedere" la lettera (Barriera 1)
            for (int p = 0; p < par->n_players; ++p) {
                mac_sem_post(&shared->players_sem[p]);
            }

            // Aspetta che tutti i Player abbiano fatto le loro valutazioni (Barriera 2)
            for (int p = 0; p < par->n_players; ++p) {
                mac_sem_wait(&shared->dealer_sem);
            }

            // Il Dealer aggiorna il tabellone
            pthread_mutex_lock(&shared->mutex);
            int occorrenze = update_table_and_count(shared->frase_offuscata, shared->frase_in_chiaro, shared->lettera_estratta);
            pthread_mutex_unlock(&shared->mutex);

            if (occorrenze > 0) {
                printf("[DEALER] Ottimo! La lettera '%c' compare %d volte!\n", shared->lettera_estratta, occorrenze);
                printf("[TABELLONE]: %s\n", shared->frase_offuscata);
                
                // Assegna punti a un giocatore casuale (simuliamo che un giocatore abbia chiamato la lettera)
                int player_fortunato = rand() % par->n_players;
                shared->punteggi[player_fortunato] += (occorrenze * 100);
                printf("[DEALER] Il Giocatore %d guadagna %d punti!\n\n", player_fortunato + 1, occorrenze * 100);
            } else {
                printf("[DEALER] Peccato, la lettera '%c' non e' presente.\n\n", shared->lettera_estratta);
            }

            // Condizione di Vittoria (Tabellone svelato)
            if (strcmp(shared->frase_in_chiaro, shared->frase_offuscata) == 0) {
                printf("+++ LA FRASE E' STATA INDOVINATA! +++\n");
                printf("Soluzione: %s\n", shared->frase_in_chiaro);
                break;
            }
        }
    }

    // Terminazione e Poison Pill
    pthread_mutex_lock(&shared->mutex);
    shared->game_over = 1;
    pthread_mutex_unlock(&shared->mutex);
    
    // Sveglia i player un'ultima volta per farli uscire dai loro while(1)
    for (int p = 0; p < par->n_players; ++p) {
        mac_sem_post(&shared->players_sem[p]);
    }

    free(frasi);
    free(args);
    return NULL;
}

void* players_func(void* args) {
    Struct_Shared_Thread* thread_info = (Struct_Shared_Thread*) args;
    Struct_Shared* shared = thread_info->shared;
    int id = thread_info->id;

    while (1) {
        // Aspetta che il Dealer giri la ruota
        mac_sem_wait(&shared->players_sem[id]);

        // Controllo Terminazione
        pthread_mutex_lock(&shared->mutex);
        if (shared->game_over) {
            pthread_mutex_unlock(&shared->mutex);
            break;
        }
        
        // Il giocatore guarda la lettera
        // printf("[Player-%d] Vedo la lettera '%c', spero ci sia!\n", id + 1, shared->lettera_estratta);
        pthread_mutex_unlock(&shared->mutex);

        // Conferma al Dealer di essere pronto
        mac_sem_post(&shared->dealer_sem);
    }
    
    free(args);
    return NULL;
}

// MAIN
int main(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "Uso: %s <numero_giocatori> <numero_partite> <file_frasi.txt>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    srand((unsigned int)time(NULL));

    Parametri par;
    par.n_players = atoi(argv[1]);
    par.m_match = atoi(argv[2]);
    strcpy(par.file_name, argv[3]);

    Struct_Shared* shared = (Struct_Shared*) malloc(sizeof(Struct_Shared));
    shared->game_over = 0;
    shared->punteggi = (int*) calloc(par.n_players, sizeof(int)); // Array inizializzato a 0

    shared->players_sem = (mac_sem_t*) malloc(sizeof(mac_sem_t) * par.n_players);
    for (int i = 0; i < par.n_players; ++i) {
        mac_sem_init(&shared->players_sem[i], 0, 0);
    }
    mac_sem_init(&shared->dealer_sem, 0, 0);
    pthread_mutex_init(&shared->mutex, NULL);

    pthread_t thread_dealer;
    pthread_t thread_players[par.n_players];

    Struct_Shared_Dealer* dealer_args = (Struct_Shared_Dealer*) malloc(sizeof(Struct_Shared_Dealer));
    dealer_args->shared = shared;
    dealer_args->par = &par;
    pthread_create(&thread_dealer, NULL, dealer_func, dealer_args);

    for (int i = 0; i < par.n_players; ++i) {
        Struct_Shared_Thread* player_args = (Struct_Shared_Thread*) malloc(sizeof(Struct_Shared_Thread));
        player_args->shared = shared;
        player_args->id = i; 
        pthread_create(&thread_players[i], NULL, players_func, player_args);
    }

    // Join
    pthread_join(thread_dealer, NULL);
    for (int i = 0; i < par.n_players; ++i) {
        pthread_join(thread_players[i], NULL);
    }

    // Risultato Finale
    printf("\n==================================================\n");
    printf("               PUNTEGGI FINALI\n");
    printf("==================================================\n");
    int max_punteggio = -1;
    int vincitore = -1;
    for (int i = 0; i < par.n_players; ++i) {
        printf("Giocatore %d: %d punti\n", i + 1, shared->punteggi[i]);
        if (shared->punteggi[i] > max_punteggio) {
            max_punteggio = shared->punteggi[i];
            vincitore = i + 1;
        }
    }
    printf("--------------------------------------------------\n");
    printf("IL VINCITORE E' IL GIOCATORE %d CON %d PUNTI!\n", vincitore, max_punteggio);
    printf("==================================================\n");

    // Distruzione
    pthread_mutex_destroy(&shared->mutex);
    mac_sem_destroy(&shared->dealer_sem);
    for (int i = 0; i < par.n_players; ++i) {
        mac_sem_destroy(&shared->players_sem[i]);
    }
    
    free(shared->punteggi);
    free(shared->players_sem);
    free(shared);

    return EXIT_SUCCESS;
}
