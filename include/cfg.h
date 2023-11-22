#pragma once

#include <stdlib.h>
#include <inttypes.h>

enum cfg_setting_type_e {
    cfg_setting_type_integer,
    cfg_setting_type_decimal,
    cfg_setting_type_string,
    cfg_setting_type_boolean,
};

typedef struct cfg_setting_s {
    enum cfg_setting_type_e type;
    char* identifier;

    union {
        long long integer;
        long double decimal;
        char* string;
        bool boolean;
    };
} cfg_setting_t;

typedef struct cfg_s {
    char* path;
    cfg_setting_t** settings;
    size_t settings_len;
} cfg_t;

int cfg_load(cfg_t* cfg, const char* path);
void cfg_free(cfg_t* cfg);

bool cfg_get_boolean_setting(cfg_t* cfg, const char* identifier);
char* cfg_get_string_setting(cfg_t* cfg, const char* identifier);
long long cfg_get_integer_setting(cfg_t* cfg, const char* identifier);
long double cfg_get_decimal_setting(cfg_t* cfg, const char* identifier);

void* cfg_get_setting(cfg_t* cfg, const char* identifier);

void cfg_dump(cfg_t* cfg);