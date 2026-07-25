#ifndef NATIVE_TASK_H
#define NATIVE_TASK_H

// Interface, that needs to be implemented by all
// task plugins.

// This is the entry point of the actual task.
// The init orchestratpr calls this function to execute the task.
// Upon successful execution it should return 0.
int native_task_run(void);

#endif // NATIVE_TASK_H
