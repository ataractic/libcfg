# libcfg
robust and minimalistic configuration file parser. keeps track of syntax errors, supports `boolean values`, `integers`, `floating point numbers` and `strings`.

## example

the following example shows how to use all libcfg API functions. try this example [here](tests/run-test_1.sh).

```toml
# test_1.cfg

my_string = "mystère"
my_int = 1337 # leet
my_bool = false
my_double = 3.456789
```

```c
/* test_1.c */

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
```

## feedback

i'm open to feedback and improvements! please create [an issue](https://github.com/eretsym/libcfg/issues/new) for this purpose.