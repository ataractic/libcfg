#include <stdio.h>
#include "../include/cfg.h"

int main() {
    int status = 0;
    cfg_t cfg; /* = {0} */
    long long my_int;
    long double my_double;
    bool my_bool;
    char* my_string;

    /* initializes the object */
    cfg_init(&cfg);

    /* loads a config file and gets its content. */
    if (cfg_load(&cfg, "./test_1.cfg") != 0) {
        /* prints the reason of the fail */
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }

    /* it is also possible to load a config from a buffer */
    if (cfg_parse(&cfg, "buffer.content=808", 18) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }

    /* get the setting values (this does not make a copy) */
    if (cfg_get_setting(&cfg, "my_string", &my_string) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }
    if (cfg_get_setting(&cfg, "my_int", &my_int) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }
    if (cfg_get_setting(&cfg, "my_double", &my_double) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }
    if (cfg_get_setting(&cfg, "my_bool", &my_bool) != 0) {
        printf("cfg: error: %s\n", cfg_get_last_error(&cfg));
        status = 1;
        goto main_free;
    }

    /* dumps the config */
    cfg_dump(&cfg);

main_free:
    /* free the cfg object and remove the settings */
    cfg_free(&cfg);

    return status;
}