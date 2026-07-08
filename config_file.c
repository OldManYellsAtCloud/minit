//open
#include <sys/stat.h>
#include <fcntl.h>

//fprintf
#include <stdio.h>

//errno
#include <errno.h>

//streeror
#include <string.h>

//malloc
#include <stdlib.h>

// fts
#include <fts.h>

// close
#include <unistd.h>

#include "config_file.h"
#include "text_utils.h"

#define ONE_MB  1024 * 1024
#define ONE_LINE 1024

static int id = 0;

int config_file_parse(const char *path, struct config_file *cfg) {
    int ret = 0;
    size_t line_size = ONE_LINE;
    FILE *cfg_stream;
    char *buf = NULL;
    ssize_t bytes_read;

    cfg_stream = fopen(path, "r");
    if (!cfg_stream){

        fprintf(stderr, "Could not open cfg: %s. %d: %s\n", path,
                errno, strerror(errno));

        ret = -1;
        goto exit;
    }

    struct stat st;
    if (fstat(fileno(cfg_stream), &st) < 0){
        fprintf(stderr, "Could not stat cfg: %s. %d: %s\n", path,
                errno, strerror(errno));
        ret = -1;
        goto cleanup;
    }

    if (st.st_size > ONE_MB) {
        fprintf(stderr, "Too big config, should be smaller than 1MB: %s\n",
                path);
        ret = -1;
        goto cleanup;
    }

    cfg->id = id++;
    cfg->rem_deps_num = 0; // if it has no deps, this won't be modified.
    cfg->path = strdup(path);
    cfg->complete = false;
    cfg->in_progress = false;
    cfg->timeout = CONFIG_FILE_TIMEOUT_INFINITE;

    errno = 0;
    while ((bytes_read = getline(&buf, &line_size, cfg_stream)) > 0){
        if (strncmp("cmd: ", buf, 4) == 0){
            char* full_cmd = text_utils_trim(text_utils_get_config_value(buf));
            if (config_file_string_to_task_spec(full_cmd, cfg->task) != 0){
                fprintf(stderr, "Could not parse command: %s\n", full_cmd);
                ret = -1;
                goto cleanup;
            }
            continue;
        }

        if (strncmp("dependencies: ", buf, 14) == 0){
            cfg->deps = text_utils_trim(text_utils_get_config_value(buf));
            cfg->rem_deps_num = text_utils_count_words(cfg->deps);
            continue;
        }

        if (strncmp("timeout: ", buf, 9) == 0){
            errno = 0;
            long timeout = strtol(text_utils_get_config_value(buf), NULL, 10);
            if (errno){
                fprintf(stderr, "Could not parse timeout from line: %s. Error: %d - %s\n",
                        buf, errno, strerror(errno));
                continue;
            }
            cfg->timeout = timeout;
            continue;
        }
        fprintf(stderr, "Unrecognized config line: %s: %s\n", path, buf);
    }

    if (bytes_read < 0 && errno != 0){
        fprintf(stderr, "Getlines error: %s - %d: %s\n", path, errno, strerror(errno));
        ret = -1;
    }

cleanup:
    free(buf);
    fclose(cfg_stream);

exit:
    return ret;
}

struct config_file* config_file_parse_all(char *folder_path){
    struct config_file *ret;
    struct stat st;

    FTS *fts;
    FTSENT *ftsent;

    size_t ret_cnt = 0;
    size_t ret_size = 10;

    if (stat(folder_path, &st) != 0){
        fprintf(stderr, "Could not open cfg folder: %s - %d: %s\n", folder_path,
                errno, strerror(errno));
        return NULL;
    }

    if (!S_ISDIR(st.st_mode)){
        fprintf(stderr, "Config path %s is not a folder.\n", folder_path);
        return NULL;
    }

    char *f[] = {folder_path, NULL};

    fts = fts_open(f, FTS_PHYSICAL, NULL);
    ret = malloc(ret_size * sizeof(struct config_file));

    while ((ftsent = fts_read(fts)) != NULL){
        if (!(ftsent->fts_info & FTS_F)){
            printf("%s is not a file, skipping\n", ftsent->fts_path);
            continue;
        }

        if (ret_cnt == (ret_size - 1)){
            ret_size *= 2;
            struct config_file *tmp = realloc(ret, ret_size * sizeof(struct config_file));
            if (!tmp){
                fprintf(stderr, "CRITICAL: Not enough memory to store %lu config structs\n", ret_size);
                free(ret);
                ret = NULL;
                goto exit;
            }
            ret = tmp;
        }

        config_file_init(&ret[ret_cnt]);

        if (config_file_parse(ftsent->fts_path, &ret[ret_cnt]) < 0){
            printf("Could not parse %s, skipping\n", ftsent->fts_path);
            continue;
        }

        ++ret_cnt;
    }

    ret[ret_cnt] = END_OF_ARRAY;

exit:
    fts_close(fts);
    return ret;
}

void config_file_dump(const struct config_file *cfg){
    if (!cfg){
        printf("Warning: cfg is null.\n");
        return;
    }

    printf("id: %d\ncmd (w/o args): %s\npath to cfg: %s\ndeps_num: %d\ndeps: %s\n",
            cfg->id, cfg->task->cmd, cfg->path, cfg->rem_deps_num,
            cfg->rem_deps_num ? cfg->deps : "-");

    printf("=======\n");
}

void config_file_dump_not_finished(const struct config_file *cfg_array) {
    for (size_t i = 0; cfg_array[i].id != END_OF_ARRAY.id; ++i){
        if (!cfg_array[i].complete)
            config_file_dump(&cfg_array[i]);
    }
}

int config_file_get_next(struct config_file *cfg_array, struct config_file **dest) {
    bool there_is_still_pending = false;
    for (size_t i = 0; cfg_array[i].id != END_OF_ARRAY.id; ++i){
        if (cfg_array[i].rem_deps_num == 0 && !cfg_array[i].complete && !cfg_array[i].in_progress){
            *dest = &cfg_array[i];
            return CONFIG_FILE_DONE;
        }
        if (cfg_array[i].rem_deps_num > 0)
            there_is_still_pending = true;
    }

    return there_is_still_pending ? CONFIG_FILE_PENDING : CONFIG_FILE_FINISHED;
}

int config_file_dependency_present(struct config_file *cfg, const char* dependency){
    if (cfg->deps == NULL)
        return 1;
    const char* result = strstr(cfg->deps, dependency);
    return result ? 0 : 1;
}

int config_file_finish_dependency(struct config_file *cfg, const char* dependency) {
    char *new_dep;
    char *dep_start = strstr(cfg->deps, dependency);
    if (!dep_start){
        printf("Cfg %s does not depend on %s\n", cfg->path, dependency);
        return -1;
    }

    size_t start = dep_start - cfg->deps;
    size_t end = start + strlen(dependency);
    size_t new_dep_size = strlen(cfg->deps) - strlen(dependency) + 1;

    new_dep = malloc(new_dep_size);
    if (!new_dep){
        fprintf(stderr, "Not enough memory to handle dependencies!");
        return -1;
    }

    // skip the middle with the substring
    memcpy(new_dep, cfg->deps, start);
    memcpy(new_dep + start, cfg->deps + end, strlen(cfg->deps) - end);
    new_dep[new_dep_size - 1] = '\0';

    free(cfg->deps);

    cfg->deps = text_utils_trim(new_dep);
    free(new_dep);

    cfg->rem_deps_num--;
    return 0;
}

int config_file_string_to_task_spec(char* full_cmd, struct task_spec* ts) {
    char* token;
    char** args = NULL;
    char** tmp;
    size_t arg_num = 0;

    // strtok modifies its argument
    // which not only breaks it for later usage, but also needs to be in
    // a writable memory location, making it testing harder without
    // a copy that's surely writeable.
    char* cmd_copy = strdup(full_cmd);

    token = strtok(cmd_copy, " ");
    if (token == NULL){
        fprintf(stderr, "Could not parse cmd from full_cmd: %s\n", full_cmd);
        return -1;
    }
    ts->cmd = strdup(token);

    // need at least an initial malloc, in case there are no arguments
    args = malloc(sizeof(char*));

    while ((token = strtok(NULL, " ")) != NULL){
        tmp = realloc(args, (arg_num + 1) * sizeof(char*));
        if (tmp == NULL){
            fprintf(stderr, "Could not realloc args array: %d - %s\n", errno, strerror(errno));
            free(args);
            return -1;
        }
        args = tmp;
        args[arg_num] = strdup(token);
        ++arg_num;
    }

    args[arg_num] = NULL;
    ts->args = args;

    return 0;
}

void config_file_dependency_done(struct config_file* cfg_arr, const char* dependency){
    size_t idx = 0;
    while (cfg_arr[idx].id != END_OF_ARRAY.id){
        if (config_file_dependency_present(&cfg_arr[idx], dependency) == 0){
            config_file_finish_dependency(&cfg_arr[idx], dependency);
        }
        ++idx;
    }
}

int config_file_init(struct config_file* cf){
    cf->task = malloc(sizeof(struct task_spec));
    if (cf->task == NULL){
        fprintf(stderr, "Could not alloc memory: %d - %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}
