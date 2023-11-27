#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

/**
 * @brief error codes
*/
enum cfg_error_e {
    cfg_success,
    cfg_error_mem,
    cfg_error_invint,
    cfg_error_invfloat,
    cfg_error_invstring,
    cfg_error_invbool,
    cfg_error_invnull,
    cfg_error_invid,
    cfg_error_range,
    cfg_error_apply,
    cfg_error_parse,
    cfg_error_open,
    cfg_error_size,
    cfg_error_seek,
    cfg_error_map,
    cfg_error_nexist,
    cfg_error_huh,
};

/**
 * @brief supported setting types
*/
enum cfg_setting_type_e {
    cfg_setting_type_integer,
    cfg_setting_type_floating,
    cfg_setting_type_string,
    cfg_setting_type_boolean,
};

/**
 * @brief setting object, containing the setting value
*/
typedef struct cfg_setting_s {
    enum cfg_setting_type_e type;
    char* identifier;

    union {
        long long integer;
        long double floating;
        char* string;
        bool boolean;
    };
} cfg_setting_t;

/**
 * @brief cfg object
*/
typedef struct cfg_s {
    char* path;
    cfg_setting_t** settings;
    size_t settings_len;
    size_t line;
    size_t col;
} cfg_t;

extern int cfg_errno;
extern cfg_t cfg_g;

const char *cfg_strerror(int errnum);
void cfg_perror(const char *error_string);

int cfg_parse(const char* str, size_t len);
int cfg_load(const char* path);
void cfg_free(void);

int cfg_get_setting(const char* identifier, void* value);
enum cfg_setting_type_e cfg_get_setting_type(const char* identifier);

void cfg_dump(void);
size_t cfg_get_error_line(void);
size_t cfg_get_error_col(void);
const char* cfg_get_path(void);