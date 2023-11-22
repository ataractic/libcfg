#include <stdio.h>
#include "../include/cfg.h"

typedef struct my_config_s {
    int my_int;
    double my_double;
    char* my_string;
    float my_float;
    long my_long;
    unsigned char* my_utf8_string;
    long long my_long_long;
    unsigned long long my_unsigned_long_long;
} my_config_t;

int main() {
    cfg_t cfg;

    if (cfg_load(&cfg, "./test_1.cfg") != 0) {
        return 1;
    }

    printf("dump:\n");
    cfg_dump(&cfg);

    printf("individual values:\n");
    printf("boolean=%d\n", cfg_get_boolean_setting(&cfg, "boolean"));
    printf("string=%s\n", cfg_get_string_setting(&cfg, "string"));

    cfg_free(&cfg);

    return 0;
}