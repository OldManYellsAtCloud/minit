#include "task.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

void* task_execute(void *task){
    pid_t fork_pid;
    struct config_file *cf = (struct config_file*)task;
    if ((fork_pid = fork()) == 0){
        execv(cf->task->cmd, cf->task->args);
    } else {

    }

}


int task_run_task(struct config_file *cf){
    pthread_t pt;

    if (pthread_create(&pt, NULL, task_execute, cf) != 0) {
        fprintf(stderr, "Could not create thread: %d - %s\n", errno, strerror(errno));
    }
}