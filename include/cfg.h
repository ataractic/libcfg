#pragma once

#include <stdlib.h>
#include <inttypes.h>

enum cfg_setting_type_e {
    cfg_setting_type_integer,
    cfg_setting_type_floating,
    cfg_setting_type_string,
    cfg_setting_type_boolean,
};

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

typedef struct cfg_s {
    char* last_error;
    char* path;
    cfg_setting_t** settings;
    size_t settings_len;
} cfg_t;

int cfg_load(cfg_t* cfg, const char* path);
void cfg_free(cfg_t* cfg);

int cfg_get_setting(cfg_t* cfg, const char* identifier, void* value);
char* cfg_get_last_error(cfg_t* cfg);

void cfg_dump(cfg_t* cfg);