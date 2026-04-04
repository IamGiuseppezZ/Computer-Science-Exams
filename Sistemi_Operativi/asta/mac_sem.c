// mac_sem.c
#include "mac_sem.h"
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>

int mac_sem_init(mac_sem_t *psem, int flags, unsigned count) {
    if (pthread_mutex_init(&psem->count_lock, NULL) != 0)
        return -1;
    if (pthread_cond_init(&psem->count_bump, NULL) != 0) {
        pthread_mutex_destroy(&psem->count_lock);
        return -1;
    }
    psem->count = count;
    return 0;
}

int mac_sem_destroy(mac_sem_t *psem) {
    pthread_mutex_destroy(&psem->count_lock);
    pthread_cond_destroy(&psem->count_bump);
    return 0;
}

int mac_sem_post(mac_sem_t *psem) {
    pthread_mutex_lock(&psem->count_lock);
    psem->count++;
    pthread_cond_signal(&psem->count_bump);
    pthread_mutex_unlock(&psem->count_lock);
    return 0;
}

int mac_sem_wait(mac_sem_t *psem) {
    pthread_mutex_lock(&psem->count_lock);
    while (psem->count == 0)
        pthread_cond_wait(&psem->count_bump, &psem->count_lock);
    psem->count--;
    pthread_mutex_unlock(&psem->count_lock);
    return 0;
}

int mac_sem_trywait(mac_sem_t *psem) {
    int result = 0;
    pthread_mutex_lock(&psem->count_lock);
    if (psem->count > 0)
        psem->count--;
    else
        result = -1;
    pthread_mutex_unlock(&psem->count_lock);
    return result;
}

int mac_sem_timedwait(mac_sem_t *psem, const struct timespec *abstim) {
    int result = 0;
    pthread_mutex_lock(&psem->count_lock);
    while (psem->count == 0) {
        result = pthread_cond_timedwait(&psem->count_bump, &psem->count_lock, abstim);
        if (result != 0)
            break;
    }
    if (result == 0)
        psem->count--;
    pthread_mutex_unlock(&psem->count_lock);
    return result;
}

int mac_sem_getvalue(mac_sem_t *sem, int *sval) {
    pthread_mutex_lock(&sem->count_lock);
    *sval = sem->count;
    pthread_mutex_unlock(&sem->count_lock);
    return 0;
}
