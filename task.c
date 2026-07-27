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
#include <sys/wait.h>

#include <dlfcn.h>
#include <linux/pidfd.h>
#include <stdint.h>

#ifdef BENCHMARK
#include <time.h>
#endif

static int cur_jobs = 0;

static bool keep_going = true;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cnd = PTHREAD_COND_INITIALIZER;

static int jobs_done = 0;
static struct config_file **done_list;

static void* task_run_native_task(void *config_file){
    void *dyn_handle;
    typeof(int(void)) *dyn_task;
    struct config_file *cf = (struct config_file*)config_file;
    int *ret = malloc(sizeof(int));

    dyn_handle = dlopen(cf->task->cmd, RTLD_LAZY | RTLD_LOCAL);
    if (!dyn_handle){
        fprintf(stderr, "Error opening native task file %s: %s\n", cf->task->cmd, dlerror());
        *ret = -1;
        goto native_task_exit;
    }

    dyn_task = dlsym(dyn_handle, "run");
    if (!dyn_task){
        fprintf(stderr, "Error querying run method in %s: %s\n", cf->task->cmd, dlerror());
        *ret = -1;
        goto native_task_exit;
    }
    *ret = dyn_task();

native_task_exit:
    return ret;
}

// for native tasks, without fork+exec
static int task_run_thread(struct config_file *cf, pthread_t *pt){
    *pt = pthread_create(pt, NULL, task_run_native_task, cf);
    if (!pt){
        fprintf(stderr, "Could not create thread: %d - %s\n", errno, strerror(errno));
        return -1;
    }

    return 0;
}

// for script tasks, with fork+exec
int task_run_fork(struct config_file *cf, pid_t *fork_pid){
    if ((*fork_pid = fork()) == 0){
        if (execvp(cf->task->cmd, cf->task->args) == -1){
            fprintf(stderr, "FATAL: execv failed: %d - %s\n", errno, strerror(errno));
            return -1;
        }
    }
    printf("Fork successful: %d\n", *fork_pid);
    return 0;
}

void* task_execute(void *task){
    pid_t fork_pid;
    pthread_t pt;
    int pidfd;
    int ret;
    int timeout;
    struct pollfd pfd;
    void *dyn_handle;
    typeof(int(void)) *dyn_task;

#ifdef BENCHMARK
    struct timespec start, end;
    uint64_t exec_time;
    if (clock_gettime(CLOCK_MONOTONIC, &start) != 0){
        fprintf(stderr, "Could not get start time: %d - %s\n", errno, strerror(errno));
    }
#endif

    struct config_file *cf = (struct config_file*)task;

    switch(cf->ttype){
    case SCRIPT:
        if (task_run_fork(cf, &fork_pid) == -1){
            keep_going = false;
            goto exit;
        }
        pidfd = syscall(SYS_pidfd_open, fork_pid, 0);
        break;
    case NATIVE:
        if (task_run_thread(cf, &pt) == -1){
            keep_going = false;
            goto exit;
        }
        pidfd = syscall(SYS_pidfd_getfd, pt, PIDFD_THREAD);
        break;
    }

    if (pidfd < 0){
        fprintf(stderr, "Fatal: could not open pidfd for task: %s: %d - %s\n",
                cf->task->cmd, errno, strerror(errno));
        keep_going = false;
        goto exit;
    }

    timeout = cf->timeout * 1000;

    pfd.fd = pidfd;
    pfd.events = POLLIN;

    ret = poll(&pfd, 1, timeout);

#ifdef BENCHMARK
    if (clock_gettime(CLOCK_MONOTONIC, &end) != 0){
        fprintf(stderr, "Could not get end time: %d - %s\n", errno, strerror(errno));
    } else {
        exec_time = end.tv_sec * 1000000000 + end.tv_nsec -
                    (start.tv_sec * 1000000000 + start.tv_nsec);

        printf("Exec time: %.9f s\n", exec_time / 1000000000.0);
        if (exec_time < 0){
            printf("Negative exec time! start.s: %ld, start.ns: %ld\nend.s: %ld end.ns: %ld\n",
            start.tv_sec, start.tv_nsec, end.tv_sec, end.tv_nsec);
        }
    }
#endif

    switch (ret){
    case 0:
        fprintf(stderr, "Task timeout: %s after %ld seconds\n", cf->task->cmd, cf->timeout);
        switch (cf->ttype){
        case SCRIPT:
            if (kill(fork_pid, SIGKILL) != 0){
                fprintf(stderr, "Could not kill pid: %d. %d - %s\n", fork_pid, errno, strerror(errno));
            }
            break;
        case NATIVE:
            if (pthread_cancel(pt) != 0){
                fprintf(stderr, "Could not cancel tid: %d - %s\n", errno, strerror(errno));
            }
            break;
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

    if (cf->ttype == SCRIPT)
        waitpid(fork_pid, NULL, 0);
    else
        pthread_join(pt, NULL);

exit:
    return NULL;
}

int task_do_init(struct config_file *cf_array, int job_num){
    struct config_file **cf;
    int ret = 0;
    int next_ret;
    int orig_jobs_done;

    cf = malloc(sizeof(struct config_file*));
    if (cf == NULL){
        fprintf(stderr, "Could not malloc cf: %d - %s\n", errno, strerror(errno));
        return -1;
    }

    done_list = malloc(job_num * sizeof(struct config_file*));
    if (done_list == NULL){
        fprintf(stderr, "Could not malloc done_list: %d - %s\n", errno, strerror(errno));
        free(cf);
        return -1;
    }

    while (keep_going){
        // if there are any jobs done, remove them from the dependency list
        if (jobs_done > 0){
            pthread_mutex_lock(&mtx);
            while (jobs_done > 0){
                config_file_dependency_done(cf_array, done_list[--jobs_done]->path);
            }
            pthread_mutex_unlock(&mtx);
        }

        if (cur_jobs == job_num){
            pthread_mutex_lock(&mtx);
            while (cur_jobs == job_num)
                pthread_cond_wait(&cnd, &mtx);
            pthread_mutex_unlock(&mtx);
        }

        next_ret = config_file_get_next(cf_array, cf);

        // there are no more tasks to process, we are done
        if (next_ret == CONFIG_FILE_FINISHED){
            keep_going = false;
            continue;
        }

        // there are tasks, but still waiting for dependencies - but there are
        // no more running jobs. deadlock.
        if (next_ret == CONFIG_FILE_PENDING && cur_jobs == 0 && jobs_done == 0){
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

        if (task_run_task(*cf) != 0){
            fprintf(stderr, "Could not run task\n");
            break;
        }
        printf("Running task:\n");
        config_file_dump(*cf);
        (*cf)->in_progress = true;
    }

    // wait for the last jobs
    pthread_mutex_lock(&mtx);
    while (cur_jobs > 0)
        pthread_cond_wait(&cnd, &mtx);
    pthread_mutex_unlock(&mtx);

    free(done_list);
    free(cf);
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