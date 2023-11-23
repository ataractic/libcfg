#include <stdio.h>
#include "../include/cfg.h"

typedef struct my_config_s {
    long long my_int;
    long double my_double;
    bool my_bool;
    char* my_string;
} my_config_t;

int main() {
    int status = 0;
    cfg_t cfg;
    my_config_t my_config;

    /* loads a config file and gets its content. */
    if (cfg_load(&cfg, "./test_1.cfg") != 0) {
        /* prints the reason of the fail */
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }

    /* get the setting values (this is not a copy) */
    if (cfg_get_setting(&cfg, "my_string", &my_config.my_string) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }
    if (cfg_get_setting(&cfg, "my_int", &my_config.my_int) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }
    if (cfg_get_setting(&cfg, "my_double", &my_config.my_double) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }
    if (cfg_get_setting(&cfg, "my_bool", &my_config.my_bool) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }

    /* use the contents */
    printf("string=%s, bool=%s, int=%lld, float=%llf\n",
        my_config.my_string,
        my_config.my_bool ? "true" : "false",
        my_config.my_int,
        my_config.my_double
    );

main_free:
    /* free the cfg object and remove the settings */
    cfg_free(&cfg);

    return status;
}