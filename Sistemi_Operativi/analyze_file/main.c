// mac_sem.h
#ifndef MAC_SEM_H
#define MAC_SEM_H

#include <pthread.h>
#include <time.h>

typedef struct {
    unsigned int count;
    pthread_mutex_t count_lock;
    pthread_cond_t count_bump;
} mac_sem_t;

int mac_sem_init(mac_sem_t *psem, int flags, unsigned count);
int mac_sem_destroy(mac_sem_t *psem);
int mac_sem_post(mac_sem_t *psem);
int mac_sem_wait(mac_sem_t *psem);
int mac_sem_trywait(mac_sem_t *psem);
int mac_sem_timedwait(mac_sem_t *psem, const struct timespec *abstim);
int mac_sem_getvalue(mac_sem_t *sem, int *sval);

#endif
