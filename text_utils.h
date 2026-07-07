#ifndef TEXT_UTILS_H
#define TEXT_UTILS_H

#include <stddef.h>

//split line by delim, and return a pointer to the start after the delim
const char* text_utils_get_value(const char delim, const char*line);

//get value from a string with the format of "key: value"
const char* text_utils_get_config_value(const char* line);

//trim a string. the returned string needs to be free'd by the caller.
char* text_utils_trim(const char* text);

size_t text_utils_count_words(const char* text);
char* text_utils_normalize_spaces(char* text);

char** text_utils_split_line(const char* text);

#endif // TEXT_UTILS_H
