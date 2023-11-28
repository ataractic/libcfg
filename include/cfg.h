#pragma once

#include <stdbool.h>
#include <stdlib.h>
#include <inttypes.h>

/**
 * @brief error codes
*/
enum cfg_error_e {
    CFG_SUCCESS,
    CFG_EMEM,
    CFG_EINVINT,
    CFG_EINVFLOAT,
    CFG_EINVSTRING,
    CFG_EINVBOOL,
    CFG_EINVNULL,
    CFG_EINVID,
    CFG_ERANGE,
    CFG_EPARSE,
    CFG_EOPEN,
    CFG_ESIZE,
    CFG_ESEEK,
    CFG_EMAP,
    CFG_ENEXIST,
    CFG_EHUH,
};

/**
 * @brief supported setting types
*/
enum cfg_setting_type_e {
    CFG_STYPE_UNKNOWN,
    CFG_STYPE_INT,
    CFG_STYPE_FLOAT,
    CFG_STYPE_STRING,
    CFG_STYPE_BOOL,
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
