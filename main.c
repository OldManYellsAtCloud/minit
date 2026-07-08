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
#define CFG_FOLDER "/tmp/minit_cfg"
#else
#define CFG_FOLDER "/etc/minit"
#endif

static void minit_check_uid(){
    uid_t uid = getuid();
    if (uid != 0){
        printf("Warning: Not running as root. Expect some failures.\n");
    }
}

static long minit_get_job_number(int argc, char* argv[]){
    long num_jobs;
    char* env_temp;

    errno = 0;
    if (argc > 2 && strcmp(argv[1], "--minit.numjobs")){
        num_jobs = strtol(argv[2], NULL, 10);
        if (errno){
            fprintf(stderr, "Could not convert %s to job number.\n", argv[2]);
            exit(EXIT_FAILURE);
        }
    } else if ((env_temp = getenv("MINIT_JOB_NR"))){
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
    char *cfg_folder;
    size_t num_jobs;
    struct config_file *config_files;

    minit_check_uid();

    cfg_folder = getenv("MINIT_CFG_DIR");

    if (!cfg_folder)
        cfg_folder = CFG_FOLDER;

    num_jobs = minit_get_job_number(argc, argv);
    reaper_start();


    if (!(config_files = config_file_parse_all(cfg_folder))){
        fprintf(stderr, "CRITICAL: Could not parse config from %s\n", cfg_folder);
        exit(EXIT_FAILURE);
    }

    if (task_do_init(config_files, num_jobs) != 0){
        fprintf(stderr, "CRITICAL: could not execute all init scripts\n");
    }

    return 0;
}
