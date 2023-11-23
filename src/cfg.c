#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../include/cfg.h"

bool cfg_is_whitespace(char c) {
    return c == '\r' || c == ' ' || c == '\t';
}

bool cfg_is_character_in_string(const char* str, char c, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        if (str[i] == c) {
            return 1;
        }
    }

    return 0;
}

/**
 * @brief frees the config buffer
 * @param cfg pointer to the cfg object
*/
void cfg_free(cfg_t* cfg) {
    cfg_setting_t* current;

    for (size_t i = 0; i < cfg->settings_len; ++i) {
        current = cfg->settings[i];

        if (current->type == cfg_setting_type_string) {
            free(current->string);
        }

        free(current->identifier);
        free(current);
    }

    free(cfg->settings);
}

void cfg_dump(cfg_t* cfg) {
    cfg_setting_t* current;

    for (size_t i = 0; i < cfg->settings_len; ++i) {
        current = cfg->settings[i];

        switch (current->type) {
            case cfg_setting_type_string: {
                printf("%s=%s\n", current->identifier, current->string);
                break;
            }
            case cfg_setting_type_integer: {
                printf("%s=%lld\n", current->identifier, current->integer);
                break;
            }
            case cfg_setting_type_boolean: {
                printf("%s=%s\n", current->identifier, current->boolean ? "true" : "false");
                break;
            }
            case cfg_setting_type_floating: {
                printf("%s=%llf\n", current->identifier, current->floating);
                break;
            }
            
            default:
                break;
        }
    }
}

int cfg_add_setting(cfg_t* cfg, cfg_setting_t* setting) {
    if (cfg->settings_len == 0) {
        cfg->settings = malloc(sizeof(cfg_setting_t*) * (cfg->settings_len + 1)); // todo: handle error case
    } else {
        cfg->settings = realloc(cfg->settings, sizeof(cfg_setting_t*) * (cfg->settings_len + 1)); // todo: handle error case
    }
    
    cfg->settings_len += 1;
    cfg->settings[cfg->settings_len - 1] = setting;

    return 0;
}

int cfg_add_string_setting(cfg_t* cfg, const char* str, size_t str_len, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_string;
    setting->identifier = strndup(id, id_len);
    setting->string = strndup(str, str_len);

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->string);
        free(setting->identifier);
        free(setting);
        return 1;
    }

    return 0;
}

int cfg_add_boolean_setting(cfg_t* cfg, bool b, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_boolean;
    setting->identifier = strndup(id, id_len);
    setting->boolean = b;

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->identifier);
        free(setting);
        return 1;
    }

    return 0;
}

int cfg_add_floating_setting(cfg_t* cfg, long double value, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_floating;
    setting->identifier = strndup(id, id_len);
    setting->floating = value;

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->identifier);
        free(setting);
        return 1;
    }

    return 0;
}

int cfg_add_integer_setting(cfg_t* cfg, long long value, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_integer;
    setting->identifier = strndup(id, id_len);
    setting->integer = value;

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->identifier);
        free(setting);
        return 1;
    }

    return 0;
}

bool cfg_is_key_valid(const char* str, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if ((str[i] < 'a' || str[i] > 'z')
            && (str[i] < 'A' || str[i] > 'Z')
            && (str[i] < '0' || str[i] > '9')
            && str[i] != '.' && str[i] != '_' && str[i] != '-') {
            return 0;
        }
    }

    return 1;
}

bool cfg_is_number_syntax_valid(const char* str, size_t len) {
    size_t dot_count = 0;

    for (size_t i = 0; i < len; i++) {
        switch (str[i]) {
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                break;
            }
            case '.': {
                dot_count += 1;
                if (dot_count > 1) {
                    return 0;
                }
                break;
            }
            case '-': {
                if (i != 0) {
                    return 0;
                }
                break;
            }
            default: {
                return 0;
            }
        }
    }

    return 1;
}

int cfg_parse_integer(cfg_t* cfg, const char* str, size_t len, const char* id, size_t id_len) {
    int status = 0;
    char *endptr;
    long long result;
    char* copy;

    errno = 0;
    copy = strndup(str, len); /* handle case where the value is the last thing in the file */
    if (copy == NULL && errno == ENOMEM) {
        return 1;
    }

    errno = 0;
    result = strtoll(copy, &endptr, 10);
    if (endptr == copy) {
        status = 1;
        goto cfg_parse_integer_free;
    }

    if (errno == ERANGE || errno == EINVAL) {
        status = 1;
        goto cfg_parse_integer_free;
    }

    if (cfg_add_integer_setting(cfg, result, id, id_len) != 0) {
        status = 1;
    }

cfg_parse_integer_free:
    free(copy);

    return status;
}

int cfg_parse_floating(cfg_t* cfg, const char* str, size_t len, const char* id, size_t id_len) {
    int status = 0;
    char *endptr;
    long double result;
    char* copy;

    errno = 0;
    copy = strndup(str, len); /* handle case where the value is the last thing in the file */
    if (copy == NULL && errno == ENOMEM) {
        return 1;
    }

    errno = 0;
    result = strtold(copy, &endptr);
    if (endptr == copy) {
        status = 1;
        goto cfg_parse_floating_free;
    }

    if (errno == ERANGE || errno == EINVAL) {
        status = 1;
        goto cfg_parse_floating_free;
    }

    if (cfg_add_floating_setting(cfg, result, id, id_len) != 0) {
        status = 1;
    }

cfg_parse_floating_free:
    free(copy);

    return status;
}

int cfg_parse(cfg_t* cfg, const char* str, off_t len) {
    size_t c1 = 0;
    size_t c2 = 0;
    size_t id_pos = 0;
    size_t id_len = 0;
    size_t value_pos = 0;
    size_t value_len = 0;

    while (c2 < len) {
        switch (str[c1]) {
            /* forward the cursor until something meaningful */
            case '\r':
            case '\t':
            case ' ': {
                c2 += 1;
            }
            /* end of line expect an identifier */
            case '\n': {
                c2 += 1;
                if (c2 >= len) {
                    return 1;
                }
                c1 = c2;
                break;
            }
            /* forward the cursor to the end of the line */
            case '#': {
                while (str[c2] != '\n' && c2 < len) {
                    c2 += 1;
                }
                c1 = c2;
                break;
            }
            /* we have an identifier */
            default: {
                /* get the position and length of the identifier */
                while (str[c2] != '=' && c2 < len) {
                    c2 += 1;
                }
                id_pos = c1;
                id_len = c2 - c1;

                /* trim whitespaces at the end of the identifier */
                for (size_t i = 1; cfg_is_whitespace(str[c2 - i]); i++) {
                    id_len -= 1;
                }

                /* check for key validity */
                if (!cfg_is_key_valid(&str[id_pos], id_len)) {
                    return 1;
                }

                /* forward to the value skipping whitespaces */
                c2 += 1;
                while (cfg_is_whitespace(str[c2]) && c2 < len) {
                    c2 += 1;
                }
                c1 = c2;

                /* get the position and length of the value until end of line */
                while (str[c2] != '\n' && str[c2] != '#' && c2 < len) {
                    c2 += 1;
                }
                value_pos = c1;
                value_len = c2 - c1;

                /* trim whitespaces at the end of the value */
                for (size_t i = 1; cfg_is_whitespace(str[c2 - i]); i++) {
                    value_len -= 1;
                }

                switch (str[value_pos]) {
                    /* the value is a number */
                    case '-':
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        if (!cfg_is_number_syntax_valid(&str[value_pos], value_len)) {
                            return 1;
                        }
                        if (cfg_is_character_in_string(&str[value_pos], '.', value_len)) {
                            if (cfg_parse_floating(cfg, &str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                                return 1;
                            }
                        } else {
                            if (cfg_parse_integer(cfg, &str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                                return 1;
                            }
                        }              
                        break;
                    }
                    /* the value is a string */
                    case '\"': {
                        /* todo: create a real parser for string settings */
                        if (cfg_add_string_setting(cfg, &str[value_pos], value_len, &str[id_pos], id_len)) {
                            return 1;
                        }                        
                        break;
                    }
                    /* the value is a bool */
                    case 'f':
                    case 't': {
                        if (strncmp(&str[value_pos], "true", value_len) == 0) {
                            cfg_add_boolean_setting(cfg, 1, &str[id_pos], id_len);
                        } else if (strncmp(&str[value_pos], "false", value_len) == 0) {
                            cfg_add_boolean_setting(cfg, 0, &str[id_pos], id_len);
                        } else {
                            return 1;
                        }
                        break;
                    }
                    default: {
                        return 1;
                    }
                }

                c1 = c2;
                break;
            }
        }
    }

    return 0;
}

/**
 * @brief Gets file size in bytes
 * @param fd File descriptor
 * @return Size of file in bytes
 */
size_t cfg_get_file_size(int fd) {
    struct stat s;

    fstat(fd, &s);
    return s.st_size;
}

/**
 * @brief loads a supported config file into the program
 * @param cfg pointer to a cfg object (preferably initialized on the stack)
 * @param path path to the config file
 * @returns 0 on success
*/
int cfg_load(cfg_t* cfg, const char* path) {
    int status = 0;
    int fd;
    off_t raw_len;
    char *raw_ptr;

    fd = open(path, O_RDWR, 0600);
    if (fd == -1) {
        return 1;
    }

    if (cfg_get_file_size(fd) == 0) {
        status = 1;
        goto cfg_load_close_fd;
    }

    raw_len = lseek(fd, 0, SEEK_END);
    if (raw_len == -1) {
        status = 1;
        goto cfg_load_close_fd;
    }

    raw_ptr = mmap(0, raw_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (raw_ptr == MAP_FAILED) {
        status = 1;
        goto cfg_load_close_fd;
    }

    if (cfg_parse(cfg, raw_ptr, raw_len) != 0) {
        status = 1;
        cfg_free(cfg);
    }

cfg_load_unmap:
    munmap(raw_ptr, raw_len);

cfg_load_close_fd:
    close(fd);

    return status;
}

int cfg_get_setting(cfg_t* cfg, const char* identifier, void* value) {
    for (size_t i = 0; i < cfg->settings_len; i++) {
        if (strcmp(identifier, cfg->settings[i]->identifier) == 0) {
            switch (cfg->settings[i]->type) {
                case cfg_setting_type_boolean: {
                    *(bool*)value = cfg->settings[i]->boolean;
                    return 0;
                }
                case cfg_setting_type_string: {
                    *(char**)value = cfg->settings[i]->string;
                    return 0;
                }
                case cfg_setting_type_integer: {
                    *(long long*)value = cfg->settings[i]->integer;
                    return 0;
                }
                case cfg_setting_type_floating: {
                    *(long double*)value = cfg->settings[i]->floating;
                    return 0;
                }
                default: {
                    return 1;
                }
            }
        }
    }
    
    return 1;
}
