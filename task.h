#ifndef TASK_H
#define TASK_H

#include "config_file.h"

int task_run_task(struct config_file *cf);
int task_do_init(struct config_file *cf_array, int job_num);
#endif