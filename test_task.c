#define CTEST_MAIN
#include "ctest.h"
#include "config_file.h"

CTEST(task, dependency_manipulation){
    struct config_file cf;
    ASSERT_EQUAL(0, config_file_init(&cf));

    int ret = config_file_parse("./testconf.cfg", &cf);
    ASSERT_EQUAL(0, ret);
    ASSERT_EQUAL(2, cf.rem_deps_num);
    ASSERT_EQUAL(0, config_file_dependency_present(&cf, "dep1.cfg"));
    ASSERT_EQUAL(0, config_file_dependency_present(&cf, "dep2.cfg"));
    ASSERT_EQUAL(1, config_file_dependency_present(&cf, "dep3.cfg"));

    ASSERT_EQUAL(0, config_file_finish_dependency(&cf, "dep2.cfg"));
    ASSERT_EQUAL(1, cf.rem_deps_num);

    ASSERT_EQUAL(-1, config_file_finish_dependency(&cf, "xxx"));
    ASSERT_EQUAL(1, cf.rem_deps_num);

    ASSERT_EQUAL(0, config_file_finish_dependency(&cf, "dep1.cfg"));
    ASSERT_EQUAL(0, cf.rem_deps_num);
    ASSERT_NULL(cf.deps);
}

int main(int argc, const char* argv[]){
    int result = ctest_main(argc, argv);
    return result;
}