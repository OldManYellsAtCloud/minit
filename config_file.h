#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

#define CONFIG_FILE_NEW

// return codes from
#define CONFIG_FILE_DONE 0
#define CONFIG_FILE_FINISHED 1
#define CONFIG_FILE_PENDING 2

#define CONFIG_FILE_TIMEOUT_INFINITE -1

struct task_spec {
    char *cmd;
    char **args;
};

struct config_file {
    int id; // unique id
    //char *cmd; // command to run
    struct task_spec *task;
    char *path; // path to original config file
    long timeout; // in how many seconds it supposed to finish without being considered failed
    int rem_deps_num; // number of remaining dependencies
    char *deps; // for now it's a string. deps are separated by a space
                // TODO: made it something more sophisticated. Should it
                // be a list of pointers to other config files? Or just a list
                // of strings or other IDs?
    bool complete; // is the task finished?
    bool in_progress; // is the task running currently?
};

const static struct config_file END_OF_ARRAY = {
    .id = -1,
    .timeout = 0,
    .task = 0,
    .path = 0,
    .rem_deps_num = 0,
    .deps = 0
};

// parse a config file denoted by path, and put the result
// into config_file. Return 0 on success.
int config_file_parse(const char *path, struct config_file *cfg);

// parse all configs that can be found the folder denoted
// by folder_path, and put them in a config_file array,
// denoted by config_file. Return 0 on success.
struct config_file* config_file_parse_all(char *folder_path);

// helper method to display the content of a config_file struct to stdout
void config_file_dump(const struct config_file *cfg);

// dump the content of all config files that are not finished yet
void config_file_dump_not_finished(const struct config_file *cfg_array);

// return next executable task
// Return values:
// CONFIG_FILE_DONE - the next task is ready in "dest"
// CONFIG_FILE_FINISHED - there are no more tasks to run, the queue is finished
// CONFIG_FILE_PENDING - there are still tasks to run, but none of them are ready, they
//                       still wait for other (probably still running) dependencies
//                       to finish). Try again after the next task finished.
int config_file_get_next(struct config_file *cfg_array, struct config_file *dest);

// check if a task depends on another dependency name
// return 0 if "cfg" depends on "dependency"
int config_file_dependency_present(struct config_file *cfg, const char* dependency);

// remove the finished "dependency" from "cfg", and decrease the number of
// remaining dependencies.
int config_file_finish_dependency(struct config_file *cfg, const char* dependency);

// convert a cmd with argument to a task spec
// e.g. "ls -la foo/bar" becomes this:
// struct task_spec {
//   .cmd = "ls"
//   .args = ["-la", "foo/bar", NULL]
// }
int config_file_string_to_task_spec(char* full_cmd, struct task_spec* ts);

// For each config_file struct this function needs to be called to allocate
// memory for the task_spec struct. Return 0 on success.
int config_file_init(struct config_file *cf);
#endif // CONFIG_FILE_H
