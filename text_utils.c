#include "text_utils.h"

#include <string.h>
#include <ctype.h>

#include <stdio.h>

#include <stdlib.h>

//split line by delim, and return a pointer to the start after the delim
const char* text_utils_get_value(const char delim, const char*line){
    const char *delim_loc = strchr(line, delim);
    if (!delim_loc){
        return NULL;
    }

    // skip the delimiter
    ++delim_loc;

    // skip whitespaces
    while (delim_loc != NULL && isspace(*delim_loc))
        ++delim_loc;

    if (strlen(delim_loc) == 0)
        return NULL;

    return delim_loc;
}

//get value from a string with the format of "key: value"
const char* text_utils_get_config_value(const char* line){
    return text_utils_get_value(':', line);
}

//trim a string. the returned string needs to be free'd by the caller.
char* text_utils_trim(const char* text){
    size_t start = 0, end;
    while (start < strlen(text) && isspace(text[start]))
        ++start;

    end = strlen(text) - 1;
    while (end > start && isspace(text[end]))
        --end;

    char *trimmed = strndup(text + start, end - start + 1);

    return trimmed;
}

size_t text_utils_count_words(const char* text){
    if (!text)
        return -1;

    size_t len = strlen(text);

    int prev_space = 0;
    int count = 0;

    if (!len)
        return count;

    if (isspace(text[0]))
        prev_space = 1;
    else
        ++count;

    for (size_t i = 1; i < len; ++i){
        if (isspace(text[i])){
            prev_space = 1;
            continue;
        }

        if (prev_space){
            prev_space = 0;
            ++count;
        }
    }

    return count;
}

char* text_utils_normalize_spaces(char* text){
    size_t new_len = 0;
    char *trimmed = text_utils_trim(text);
    size_t len = strlen(trimmed);
    char *normalized = malloc(len + 1);
    char* shrinked_normalized = NULL;

    for (size_t i = 0; i < len; ++i){
        if (isspace(trimmed[i])){
            // i is at least 1 here, because trimmed string doesn't start with space
            // so it won't underflow
            if (!isspace(normalized[new_len - 1])){
                normalized[new_len++] = ' ';
            }
            continue;
        }

        normalized[new_len++] = trimmed[i];
    }

    normalized[new_len] = '\0';

    free(trimmed);
    if (new_len > 0 && new_len < len){
        shrinked_normalized = realloc(normalized, new_len);
    }

    if (!shrinked_normalized)
        return normalized;

    return shrinked_normalized;
}