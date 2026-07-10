#include "config_file.h"
#include "reaper.h"

#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "task.h"

#ifdef DEBUG_BUILD
#define DEFAULT_CFG_FOLDER "/tmp/minit_cfg"
#else
#define DEFAULT_CFG_FOLDER "/etc/minit"
#endif

static void minit_check_uid(){
    uid_t uid = getuid();
    if (uid != 0){
        printf("Warning: Not running as root. Expect some failures.\n");
    }
}

static long minit_get_job_number(){
    long num_jobs;
    char* env_temp;

    errno = 0;
    if ((env_temp = getenv("MINIT_JOB_NR"))){
        num_jobs = strtol(env_temp, NULL, 10);
        if (errno){
            fprintf(stderr, "Count not convert %s from environment to job number.\n", env_temp);
            exit(EXIT_FAILURE);
        }
    } else {
        num_jobs = sysconf(_SC_NPROCESSORS_ONLN) * 2;
        if (num_jobs < 0){
            fprintf(stderr, "Could not determine number of CPUs: %d - %s\n",
                    errno, strerror(errno));
        }
    }
    return num_jobs;
}

int main(int argc, char* argv[])
{
    char *cfg_folder = NULL;
    int num_jobs = -1;
    struct config_file *config_files;

#ifdef DEBUG_BUILD
    printf("minit started with arguments: ");
    for (int i = 1; i < argc; ++i)
        printf("%s ", argv[i]);
    printf("END\n");
#endif

    for (int i = 1; i < argc; ++i){
        if (strcmp(argv[i], "--minit.config-folder") == 0){
            cfg_folder = argv[++i];
        }

        if (strcmp(argv[i], "--minit.numjobs") == 0){
            errno = 0;
            num_jobs = strtol(argv[++i], NULL, 10);
            if (errno){
               fprintf(stderr, "Could not convert %s to job number.\n", argv[2]);
                exit(EXIT_FAILURE);
            }
        }
    }

    minit_check_uid();

    if (!cfg_folder)
        cfg_folder = getenv("MINIT_CFG_DIR");
    if (!cfg_folder)
        cfg_folder = DEFAULT_CFG_FOLDER;

    if (num_jobs == -1)
        num_jobs = minit_get_job_number();

    if (!(config_files = config_file_parse_all(cfg_folder))){
        fprintf(stderr, "CRITICAL: Could not parse config from %s\n", cfg_folder);
        exit(EXIT_FAILURE);
    }

    if (task_do_init(config_files, num_jobs) != 0){
        fprintf(stderr, "CRITICAL: could not execute all init scripts\n");
    }

    reaper_start();

    return 0;
}
