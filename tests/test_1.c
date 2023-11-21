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

    cfg_dump(&cfg);

    cfg_free(&cfg);

    return 0;
}