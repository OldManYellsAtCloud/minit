#include "task.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <poll.h>
#include <signal.h>
#include <stdlib.h>

static int cur_jobs = 0;

static bool keep_going = true;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;

static int jobs_done = 0;
static struct config_file **done_list;

void* task_execute(void *task){
    pid_t fork_pid;
    int pidfd;
    int ret;
    int timeout;
    struct pollfd pfd;

    struct config_file *cf = (struct config_file*)task;

    printf("Running task id: %d, cmd: %s\n", cf->id, cf->task->cmd);

    if ((fork_pid = fork()) == 0){
        execv(cf->task->cmd, cf->task->args);
        // no return here, of course
    }

    pidfd = syscall(SYS_pidfd_open, fork_pid, 0);
    if (pidfd < 0){
        fprintf(stderr, "Fatal: could not open pidfd for task: %s: %d - %s\n",
                cf->task->cmd, errno, strerror(errno));
        return NULL;
    }

    timeout = cf->timeout;

    pfd.fd = pidfd;
    pfd.events = POLLIN;

    ret = poll(&pfd, 1, timeout);

    switch (ret){
    case 0:
        fprintf(stderr, "Task timeout: %s after %d seconds\n", cf->task->cmd, cf->timeout);
        if (kill(fork_pid, SIGKILL) != 0){
            fprintf(stderr, "Could not kill pid: %d. %d - %s\n", fork_pid, errno, strerror(errno));
        }
        keep_going = false;
        pthread_cond_signal(&cnd);
        break;
    case -1:
        fprintf(stderr, "Error during polling: %d - %s\n", errno, strerror(errno));
        keep_going = false;
        pthread_cond_signal(&cnd);
        break;
    default:
        // successful execution
        pthread_mutex_lock(&mtx);
        cf->complete = true;
        done_list[jobs_done++] = cf;
        --cur_jobs;
        pthread_cond_signal(&cnd);
        pthread_mutex_unlock(&mtx);

        break;
    }
    return NULL;
}

int task_do_init(struct config_file *cf_array, int job_num){
    struct config_file cf;
    int ret = 0;
    int next_ret;
    int orig_jobs_done;

    done_list = malloc(job_num * sizeof(struct config_file*));
    if (done_list == NULL){
        fprintf(stderr, "Could not malloc done_list: %d - %s\n", errno, strerror(errno));
        return -1;
    }

    while (cur_jobs <= job_num && keep_going){
        // if there are any jobs done, remove them from the dependency list
        if (jobs_done > 0){
            pthread_mutex_lock(&mtx);
            while (jobs_done > 0){
                config_file_dependency_done(cf_array, done_list[--jobs_done]->path);
            }
            pthread_mutex_unlock(&mtx);
        }

        next_ret = config_file_get_next(cf_array, &cf);

        // there are no more tasks to process, we are done
        if (next_ret == CONFIG_FILE_FINISHED){
            keep_going = false;
            continue;
        }

        // there are tasks, but still waiting for dependencies - but there are
        // no more running jobs. deadlock.
        if (next_ret == CONFIG_FILE_PENDING && cur_jobs == 0){
            fprintf(stderr, "Deadlock!\n");
            ret = -1;
            keep_going = false;
            continue;
        }

        // there are still tasks, but waiting for some ongoing tasks
        // wait until something finishes
        if (next_ret == CONFIG_FILE_PENDING){
            pthread_mutex_lock(&mtx);
            orig_jobs_done = jobs_done;

            while (jobs_done == orig_jobs_done && keep_going)
                pthread_cond_wait(&cnd, &mtx);

            pthread_mutex_unlock(&mtx);
            continue;
        }

        ++cur_jobs;
        if (task_run_task(&cf) != 0){
            fprintf(stderr, "Could not run task\n");
            break;
        }
    }

    free(done_list);
    return ret;
}


int task_run_task(struct config_file *cf){
    pthread_t pt;

    if (pthread_create(&pt, NULL, task_execute, cf) != 0) {
        fprintf(stderr, "Could not create thread: %d - %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}