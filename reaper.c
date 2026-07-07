#include "reaper.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <sys/wait.h>
#include <pthread.h>

// This is just a background thread that reaps all orphaned zombie processes that
// got added as a child by kernel.
void* reaper_reap_zombies(void* arg){
    while (wait(NULL) > -1);
    return NULL;
}

void reaper_start(void){
    pthread_t pt;
    if (pthread_create(&pt, NULL, reaper_reap_zombies, NULL) != 0){
        fprintf(stderr, "Could not start zombie reaper: %d - %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if (pthread_detach(pt) != 0){
        fprintf(stderr, "Could not detach zombie reaper: %d - %s\n", errno, strerror(errno));
        exit(EXIT_FAILURE);
    }
}