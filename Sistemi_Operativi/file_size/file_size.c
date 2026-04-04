#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "mac_sem.h"

#define BUFFER_SIZE 10


struct struct_parametri{
    int num_thread;
    char** argv;
};
typedef struct struct_parametri Parametri;

struct struct_shared {
    int in_dir;
    int out_stat;
    pthread_mutex_t mutex_dir;
    pthread_mutex_t mutex_main;
    mac_sem_t empty_buffer_sem;
    mac_sem_t full_buffer_sem;

};
typedef struct struct_shared Struct_Shared;

struct struct_shared_thread{
    Struct_Shared* shared;
    int id_thread;
    Parametri par;
};
typedef struct struct_shared_thread Struct_Shared_Thread;

struct struct_shared_main {
    Struct_Shared* shared;
    Parametri par;
};
typedef struct struct_shared_main Struct_Shared_Main;

void* dir_func(void* args){
    return NULL;
}

void* stat_func(void* args){
    return NULL;
}

void* main_func(void* args){
    return NULL;
}

int main(int argc, char** argv){
    //CREO STRUCT PARAMETRI
    Parametri par;
    par.num_thread = argc - 1;
    par.argv = argv;
    //CREO STRUCT SHARED
    Struct_Shared* shared = (Struct_Shared*) malloc (sizeof(Struct_Shared));
    Struct_Shared_Main* shared_main = (Struct_Shared_Main*) malloc (sizeof(Struct_Shared_Main));
    shared_main -> shared = shared;
    shared_main -> par = par;
    //MUTEX INIT
    pthread_mutex_init(&shared->mutex_dir, NULL);
    mac_sem_init(&shared->empty_buffer_sem, 0, BUFFER_SIZE);
    mac_sem_init(&shared->full_buffer_sem, 0, 0);

    //CREO THREAD
    pthread_t dir_thread[par.num_thread];
    pthread_t stat_thread;
    pthread_t main_thread;
    if (pthread_create(&stat_thread, NULL, stat_func, (Struct_Shared_Main*) shared_main) == -1){
        perror("Error creating thead.");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&main_thread, NULL, main_func, (Struct_Shared_Main*) shared_main) == -1){
        perror("Error creating thead.");
        exit(EXIT_FAILURE);
    }
    for(size_t i = 0; i < par.num_thread; ++i){
        Struct_Shared_Thread* shared_thread = (Struct_Shared_Thread*) malloc(sizeof(Struct_Shared_Thread));
        shared_thread -> shared = shared;
        shared_thread -> par = par;
        int* id = (int*) malloc (sizeof(int));
        *id = i;
        shared_thread->id_thread = *id;
        if (pthread_create(&dir_thread[i], NULL, dir_func, (Struct_Shared_Thread*) shared_thread) == -1){
            perror("Error creating thread.\n");
            exit(EXIT_FAILURE);
        }
    }
    
    //JOIN
    pthread_join(main_thread, NULL);
    pthread_join(stat_thread, NULL);
    for(size_t i = 0; i < par.num_thread; ++i){
        pthread_join(dir_thread[i], NULL);
    }

    //MUTEX DESTROY
    pthread_mutex_destroy(&shared->mutex_dir);
    mac_sem_destroy(&shared->empty_buffer_sem);
    mac_sem_destroy(&shared->full_buffer_sem);


}

//N-THREAD DIR che si occuperanno di scansionare la cartella e inserirli in un buffer[10]
//STAT - THREAD che si occuperà di determinare la dimensione di ogni file scansionato.
//MAIN THREAD che terrà in conto la dimensione totale dei file;