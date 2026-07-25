#include "native_task.h"
#include <stdio.h>

int run(void){
    printf("\n%s\n", "printing from plugin: sample native task");
    return 1;
}
