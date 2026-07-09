#define CTEST_MAIN
#include "ctest.h"
#include "config_file.h"

CTEST(config_file, string_to_task){
    struct task_spec ts;
    int ret = config_file_string_to_task_spec("a bc d e", &ts);
    ASSERT_EQUAL(0, ret);
    ASSERT_STR("a", ts.cmd);
    ASSERT_STR("a", ts.args[0]);
    ASSERT_STR("bc", ts.args[1]);
    ASSERT_STR("d", ts.args[2]);
    ASSERT_STR("e", ts.args[3]);
    ASSERT_NULL(ts.args[4]);
}

CTEST(config_file, string_to_task2){
    struct task_spec ts;
    int ret = config_file_string_to_task_spec("a ", &ts);
    ASSERT_EQUAL(0, ret);
    ASSERT_STR("a", ts.cmd);
    ASSERT_STR("a", ts.args[0]);
    ASSERT_NULL(ts.args[1]);
}

CTEST(config_file, string_to_task_one_param){
    struct task_spec ts;
    int ret = config_file_string_to_task_spec("a b", &ts);
    ASSERT_EQUAL(0, ret);
    ASSERT_STR("a", ts.cmd);
    ASSERT_STR("a", ts.args[0]);
    ASSERT_STR("b", ts.args[1]);
    ASSERT_NULL(ts.args[2]);
}

CTEST(config_file, parse_cfg){
    struct config_file cf;
    ASSERT_EQUAL(0, config_file_init(&cf));

    int ret = config_file_parse("./testconf.cfg", &cf);
    ASSERT_EQUAL(0, ret);
    ASSERT_FALSE(cf.complete);
    ASSERT_FALSE(cf.in_progress);
    ASSERT_EQUAL(15, cf.timeout);
    ASSERT_STR("one", cf.task->cmd);
    ASSERT_STR("one", cf.task->args[0]);
    ASSERT_STR("two", cf.task->args[1]);
    ASSERT_STR("three", cf.task->args[2]);
    ASSERT_STR("four", cf.task->args[3]);
    ASSERT_NULL(cf.task->args[4]);
}

int main(int argc, const char* argv[]){
    int result = ctest_main(argc, argv);
    return result;
}