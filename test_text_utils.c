#include "text_utils.h"
#include <stdio.h>

#define CTEST_MAIN
#include "ctest.h"

CTEST(text_utils, get_value){
    char* config = "key=value";
    const char* value = text_utils_get_value('=', config);
    ASSERT_STR(value, "value");
}

CTEST(text_utils, get_config_value){
    char* config = "key: value";
    const char* value = text_utils_get_config_value(config);
    ASSERT_STR(value, "value");
}

CTEST(text_utils, get_value_space_at_end){
    char* config = "key:    ";
    const char* value = text_utils_get_config_value(config);
    ASSERT_NULL(value);
}

CTEST(text_utils, get_value_one_space_at_end){
    char* config = "key: ";
    const char* value = text_utils_get_config_value(config);
    ASSERT_NULL(value);
}

CTEST(text_utils, get_single_char_value){
    char* config = "key:   a";
    const char* value = text_utils_get_config_value(config);
    ASSERT_STR(value, "a");
}

CTEST(text_utils, spaces_at_end){
    char* config = "key:    a   ";
    const char* value = text_utils_get_config_value(config);
    ASSERT_STR(value, "a   ");
}

CTEST(text_utils, trim_no_trim){
    char* text = "aaa";
    char* trimmed = text_utils_trim(text);
    ASSERT_STR(trimmed, "aaa");
}

CTEST(text_utils, trim_right_trim){
    char* text = "aaa   ";
    char* trimmed = text_utils_trim(text);
    ASSERT_STR(trimmed, "aaa");
}

CTEST(text_utils, trim_left_trim){
    char* text = "   aaa";
    char* trimmed = text_utils_trim(text);
    ASSERT_STR(trimmed, "aaa");
}

CTEST(text_utils, trim_both_sides){
    char* text = "    aaa    ";
    char* trimmed = text_utils_trim(text);
    ASSERT_STR(trimmed, "aaa");
}

CTEST(text_utils, trim_middle_space){
    char* text = "   aaa aaa   ";
    char* trimmed = text_utils_trim(text);
    ASSERT_STR(trimmed, "aaa aaa");
}

CTEST(text_utils, count_words){
    char* text = "one two three";
    size_t cnt = text_utils_count_words(text);
    ASSERT_EQUAL(3, cnt);
}

CTEST(text_utils, count_words2){
    char* text = "one";
    size_t cnt = text_utils_count_words(text);
    ASSERT_EQUAL(1, cnt);
}

CTEST(text_utils, count_words3){
    char* text = "";
    size_t cnt = text_utils_count_words(text);
    ASSERT_EQUAL(0, cnt);
}

CTEST(text_utils, normalize_spaces){
    char* text = "a  b";
    char* normalized = text_utils_normalize_spaces(text);
    ASSERT_STR("a b", normalized);
}

CTEST(text_utils, normalize_spaces2){
    char* text = "a b";
    char* normalized = text_utils_normalize_spaces(text);
    ASSERT_STR("a b", normalized);
}

CTEST(text_utils, normalize_spaces3){
    char* text = "  a  b  ";
    char* normalized = text_utils_normalize_spaces(text);
    ASSERT_STR("a b", normalized);
}

CTEST(text_utils, normalize_spaces4){
    char* text = "a  ";
    char* normalized = text_utils_normalize_spaces(text);
    ASSERT_STR("a", normalized);
}


int main(int argc, const char* argv[]){
    int result = ctest_main(argc, argv);
    return result;
}