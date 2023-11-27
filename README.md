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

typedef struct my_config_s {
    long long my_int;
    long double my_double;
    bool my_bool;
    char* my_string;
    char* my_drum_machine;
} my_config_t;

int load_my_config(my_config_t* my_config) {
    /* loads a config file and gets its content. */
    if (cfg_load("./test_1.cfg") != 0) {
        return 1;
    }

    /* it is also possible to load a config from a buffer */
    if (cfg_parse("my_drum_machine=\"909\"", 21) != 0) {
        return 1;
    }

    /* get the setting values (this does not make a copy) */
    if (cfg_get_setting("my_string", &my_config->my_string) != 0) {
        return 1;
    }
    if (cfg_get_setting("my_int", &my_config->my_int) != 0) {
        return 1;
    }
    if (cfg_get_setting("my_double", &my_config->my_double) != 0) {
        return 1;
    }
    if (cfg_get_setting("my_bool", &my_config->my_bool) != 0) {
        return 1;
    }
    if (cfg_get_setting("my_drum_machine", &my_config->my_drum_machine) != 0) {
        return 1;
    }

    /* dumps the config */
    cfg_dump();

    return 0;
}

int main(void) {
    int status = 0;
    my_config_t my_config;

    /* here we handle the configuration loading in another function */
    if (load_my_config(&my_config) != 0) {
        fprintf(stderr, "cfg: error: %s (%s:%lu:%lu)\n", cfg_strerror(cfg_errno), cfg_get_path(), cfg_get_error_line(), cfg_get_error_col());
        status = 1;
        goto main_config_free;
    }

    /* use the loaded config in the program */
    printf("hi, i am %s, %s warrior. i am %Lf cm tall and have %lld street cred. my favorite drum machine is the %s.\n", 
        my_config.my_string,
        my_config.my_bool ? "true" : "false",
        my_config.my_double,
        my_config.my_int,
        my_config.my_drum_machine
    );


main_config_free:
    /* must free the cfg object after use*/
    cfg_free(); 

    return status;
}
```

```
terminal@localhost $ ./run-test_1.sh
my_string="mystère"
my_int=1337
my_bool=true
my_double=3.140000
my_drum_machine="909"
hi, i am mystère, true warrior. i am 3.140000 cm tall and have 1337 street cred. my favorite drum machine is the 909.
```

## feedback

i'm open to feedback and improvements! please create [an issue](https://github.com/eretsym/libcfg/issues/new) for this purpose.