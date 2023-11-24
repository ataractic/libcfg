#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include "../include/cfg.h"

/**
 * @brief sets the last error string
 * @param cfg pointer to the configuration object
 * @param format format string
 * @param ... format parameters
*/
static void cfg_set_last_error(cfg_t* cfg, const char* format, ...) {
    int size;
    char* tmp;

    va_list args, args2;
    va_start(args, format);
    va_copy(args2, args);

    size = vsnprintf(NULL, 0, format, args);

    tmp = malloc(size + 1);

    if (tmp != NULL) {
        vsprintf(tmp, format, args2);
    }

    if (cfg->last_error != NULL) {
        free(cfg->last_error);
    }

    cfg->last_error = tmp;

    va_end(args2);
    va_end(args);
}

/**
 * @brief gets the pointer to the last error string
 * @param cfg pointer to the configuration object
 * @returns pointer to a string
*/
char* cfg_get_last_error(cfg_t* cfg) {
    return cfg->last_error;
}

/**
 * @brief checks wether the provided character is a whitespace
 * @param c character to check
 * @returns true if the character is a whitespace, false otherwise
*/
static bool cfg_is_whitespace(char c) {
    return c == '\r' || c == ' ' || c == '\t';
}

/**
 * @brief frees the loaded configuration
 * @param cfg pointer to the configuration object
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

    if (cfg->last_error != NULL) {
        free(cfg->last_error);
    }

    if (cfg->path != NULL) {
        free(cfg->path);
    }
}

/**
 * @brief outputs the current loaded configuration to the console
 * @param cfg pointer to the configuration object
*/
void cfg_dump(cfg_t* cfg) {
    cfg_setting_t* current;

    for (size_t i = 0; i < cfg->settings_len; ++i) {
        current = cfg->settings[i];

        switch (current->type) {
            case cfg_setting_type_string: {
                printf("%s=\"%s\"\n", current->identifier, current->string);
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
                printf("%s=%Lf\n", current->identifier, current->floating);
                break;
            }
            default: {
                break;
            }
        }
    }
}

/**
 * @brief adds a setting to the configuration
 * @param cfg pointer to the configuration object
 * @param setting pointer to the setting object
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_setting(cfg_t* cfg, cfg_setting_t* setting) {
    void* tmp;

    if (cfg->settings_len == 0) {
        tmp = malloc(sizeof(cfg_setting_t*) * (cfg->settings_len + 1));
    } else {
        tmp = realloc(cfg->settings, sizeof(cfg_setting_t*) * (cfg->settings_len + 1));
    }

    if (tmp == NULL) {
        cfg_set_last_error(cfg, "out of memory");
        return 1;
    }
    
    cfg->settings = tmp;
    cfg->settings_len += 1;
    cfg->settings[cfg->settings_len - 1] = setting;

    return 0;
}

/**
 * @brief adds an string setting to the configuration object
 * @param cfg pointer to the configuration object
 * @param str pointer to the string value
 * @param str_len length of string value
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_string_setting(cfg_t* cfg, const char* str, size_t str_len, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_string;
    setting->identifier = strndup(id, id_len);
    setting->string = strndup(str, str_len);

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->string);
        free(setting->identifier);
        free(setting);
        cfg_set_last_error(cfg, "(%.*s=%.*s): %s", id_len, id, str_len, str, cfg_get_last_error(cfg));
        return 1;
    }

    return 0;
}

/**
 * @brief adds an boolean value setting to the configuration object
 * @param cfg pointer to the configuration object
 * @param b value to add
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_boolean_setting(cfg_t* cfg, bool b, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_boolean;
    setting->identifier = strndup(id, id_len);
    setting->boolean = b;

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->identifier);
        free(setting);
        cfg_set_last_error(cfg, "(%.*s=%s): %s", id_len, id, b ? "true" : "false", cfg_get_last_error(cfg));
        return 1;
    }

    return 0;
}

/**
 * @brief adds an floating point number setting to the configuration object
 * @param cfg pointer to the configuration object
 * @param value value to add
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_floating_setting(cfg_t* cfg, long double value, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_floating;
    setting->identifier = strndup(id, id_len);
    setting->floating = value;

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->identifier);
        free(setting);
        cfg_set_last_error(cfg, "(%.*s=%llf): %s", id_len, id, value, cfg_get_last_error(cfg));
        return 1;
    }

    return 0;
}

/**
 * @brief adds an integer number setting to the configuration object
 * @param cfg pointer to the configuration object
 * @param value value to add
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_integer_setting(cfg_t* cfg, long long value, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = cfg_setting_type_integer;
    setting->identifier = strndup(id, id_len);
    setting->integer = value;

    if (cfg_add_setting(cfg, setting) != 0) {
        free(setting->identifier);
        free(setting);
        cfg_set_last_error(cfg, "(%.*s=%lld): %s", id_len, id, value, cfg_get_last_error(cfg));
        return 1;
    }

    return 0;
}

/**
 * @brief checks if the given token is a valid identifier
 * @param str pointer to the serialized token
 * @param len length of the token
 * @returns true if the given token is a valid identifier, false otherwise
*/
static bool cfg_is_identifier_valid(const char* str, size_t len) {
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

/**
 * @brief checks if the given token is a valid number
 * @param str pointer to the serialized token
 * @param len length of the token
 * @returns true if the given token is a valid number, false otherwise
*/
static bool cfg_is_number_syntax_valid(const char* str, size_t len) {
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

/**
 * @brief parses an integer number from the buffer
 * @param cfg pointer to the configuration object
 * @param str pointer to the serialized value token
 * @param len length of the serialized value token
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_parse_integer(cfg_t* cfg, const char* str, size_t len, const char* id, size_t id_len) {
    int status = 0;
    char *endptr;
    long long result;
    char* copy;

    errno = 0;
    copy = strndup(str, len); /* handle case where the value is the last thing in the file */
    if (copy == NULL && errno == ENOMEM) {
        cfg_set_last_error(cfg, "out of memory");
        return 1;
    }

    errno = 0;
    result = strtoll(copy, &endptr, 10);
    if (endptr == copy) {
        cfg_set_last_error(cfg, "invalid number");
        status = 1;
        goto cfg_parse_integer_free;
    }

    if (errno == ERANGE || errno == EINVAL) {
        cfg_set_last_error(cfg, "out of range");
        status = 1;
        goto cfg_parse_integer_free;
    }

    if (cfg_add_integer_setting(cfg, result, id, id_len) != 0) {
        cfg_set_last_error(cfg, "failed to add integer setting: %s", cfg_get_last_error(cfg));
        status = 1;
    }

cfg_parse_integer_free:
    free(copy);

    return status;
}

/**
 * @brief parses a floating point number from the buffer
 * @param cfg pointer to the configuration object
 * @param str pointer to the serialized value token
 * @param len length of the serialized value token
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_parse_floating(cfg_t* cfg, const char* str, size_t len, const char* id, size_t id_len) {
    int status = 0;
    char *endptr;
    long double result;
    char* copy;

    errno = 0;
    copy = strndup(str, len); /* handle case where the value is the last thing in the file */
    if (copy == NULL && errno == ENOMEM) {
        cfg_set_last_error(cfg, "out of memory");
        return 1;
    }

    errno = 0;
    result = strtold(copy, &endptr);
    if (endptr == copy) {
        cfg_set_last_error(cfg, "invalid number");
        status = 1;
        goto cfg_parse_floating_free;
    }

    if (errno == ERANGE || errno == EINVAL) {
        cfg_set_last_error(cfg, "out of range");
        status = 1;
        goto cfg_parse_floating_free;
    }

    if (cfg_add_floating_setting(cfg, result, id, id_len) != 0) {
        cfg_set_last_error(cfg, "failed to add floating setting: %s", cfg_get_last_error(cfg));
        status = 1;
    }

cfg_parse_floating_free:
    free(copy);

    return status;
}

/**
 * @brief parses a string from the buffer
 * @param cfg pointer to the configuration object
 * @param str pointer to the serialized value token
 * @param len length of the serialized value token
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_parse_string(cfg_t* cfg, const char* str, size_t len, const char* id, size_t id_len) {
    if (len < 2 || str[0] != '\"' || str[len - 1] != '\"') {
        cfg_set_last_error(cfg, "invalid string format");
        return 1;
    }

    if (cfg_add_string_setting(cfg, &str[1], len - 2, id, id_len) != 0) {
        cfg_set_last_error(cfg, "failed to add string setting: %s", cfg_get_last_error(cfg));
        return 1;
    }

    return 0;
}

/**
 * @brief parses the serialized configuration buffer
 * @param cfg pointer to the cfg_t object
 * @param str pointer to the buffer containing the serialized configuration
 * @param len length of the buffer
 * @returns 0 on success, 1 otherwise
*/
int cfg_parse(cfg_t* cfg, const char* str, size_t len) {
    size_t c1 = 0;
    size_t c2 = 0;
    size_t id_pos = 0;
    size_t id_len = 0;
    size_t value_pos = 0;
    size_t value_len = 0;
    size_t line = 1;
    size_t col = 1;

    while (c2 < len) {
        switch (str[c2]) {
            /* forward the cursor until something meaningful */
            case '\r':
            case '\t':
            case ' ': {
                c2 += 1;
                col += 1;
            }
            /* end of line expect an identifier */
            case '\n': {
                c2 += 1;
                
                line += 1;
                col = 1;
                break;
            }
            /* forward the cursor to the end of the line */
            case '#': {
                while (c2 < len && str[c2] != '\n') {
                    c2 += 1;
                    col += 1;
                }
                break;
            }
            /* we have an identifier */
            default: {
                c1 = c2;
                /* get the position and length of the identifier */
                while (c2 < len && str[c2] != '=') {
                    c2 += 1;
                    col += 1;
                }
                id_pos = c1;
                id_len = c2 - c1;

                /* trim whitespaces at the end of the identifier */
                for (size_t i = 1; cfg_is_whitespace(str[c2 - i]); i++) {
                    id_len -= 1;
                }

                /* check for key validity */
                if (!cfg_is_identifier_valid(&str[id_pos], id_len)) {
                    cfg_set_last_error(cfg, "invalid identifier: %.*s (%s:%lu:%lu)", id_len, &str[id_pos], cfg->path, line, col - id_len);
                    return 1;
                }

                /* forward to the value skipping whitespaces */
                c2 += 1;
                col += 1;
                while (c2 < len && cfg_is_whitespace(str[c2])) {
                    c2 += 1;
                    col += 1;
                }
                c1 = c2;

                /* get the position and length of the value until end of line */
                while (c2 < len && str[c2] != '\n' && str[c2] != '#') {
                    c2 += 1;
                    col += 1;
                }
                value_pos = c1;
                value_len = c2 - c1;

                /* trim whitespaces at the end of the value */
                for (size_t i = 1; cfg_is_whitespace(str[c2 - i]); i++) {
                    value_len -= 1;
                }

                switch (str[value_pos]) {
                    /* the value should be a number */
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
                            cfg_set_last_error(cfg, "invalid number: %.*s (%s:%lu:%lu)", value_len, &str[value_pos], cfg->path, line, col - value_len);
                            return 1;
                        }
                        if (memchr(&str[value_pos], '.', value_len) != NULL) {
                            if (cfg_parse_floating(cfg, &str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                                cfg_set_last_error(cfg, "failed to parse floating: %.*s (%s:%lu:%lu): %s", value_len, &str[value_pos], cfg->path, line, col - value_len, cfg_get_last_error(cfg));
                                return 1;
                            }
                        } else {
                            if (cfg_parse_integer(cfg, &str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                                cfg_set_last_error(cfg, "failed to parse integer: %.*s (%s:%lu:%lu): %s", value_len, &str[value_pos], cfg->path, line, col - value_len, cfg_get_last_error(cfg));
                                return 1;
                            }
                        }              
                        break;
                    }
                    /* the value should be a string */
                    case '\"': {
                        if (cfg_parse_string(cfg, &str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                            cfg_set_last_error(cfg, "failed to parse string: %.*s (%s:%lu:%lu): %s", value_len, &str[value_pos], cfg->path, line, col - value_len, cfg_get_last_error(cfg));
                            return 1;
                        }                        
                        break;
                    }
                    /* the value should be a bool */
                    case 'f':
                    case 't': {
                        if (strncmp(&str[value_pos], "true", value_len) == 0) {
                            if (cfg_add_boolean_setting(cfg, 1, &str[id_pos], id_len) != 0) {
                                cfg_set_last_error(cfg, "failed to add boolean setting: %s", cfg_get_last_error(cfg));
                                return 1;
                            }
                        } else if (strncmp(&str[value_pos], "false", value_len) == 0) {
                            if (cfg_add_boolean_setting(cfg, 0, &str[id_pos], id_len) != 0) {
                                cfg_set_last_error(cfg, "failed to add boolean setting: %s", cfg_get_last_error(cfg));
                                return 1;
                            }
                        } else {
                            cfg_set_last_error(cfg, "invalid boolean value: %.*s (%s:%lu:%lu)", value_len, &str[value_pos], cfg->path, line, col - value_len);
                            return 1;
                        }
                        break;
                    }
                    default: {
                        cfg_set_last_error(cfg, "invalid value: (null) (%s:%lu:%lu)", cfg->path, line, col - value_len);
                        return 1;
                    }
                }
                break;
            }
        }
    }

    return 0;
}

/**
 * @brief Gets file size in bytes
 * @param fd File descriptor
 * @returns Size of file in bytes
 */
static size_t cfg_get_file_size(int fd) {
    struct stat s;

    fstat(fd, &s);
    return s.st_size;
}

/**
 * @brief loads a supported config file into the program
 * @param cfg pointer to a cfg object (preferably initialized on the stack)
 * @param path path to the config file
 * @returns 0 on success, 1 otherwise
*/
int cfg_load(cfg_t* cfg, const char* path) {
    int status = 0;
    int fd;
    off_t raw_len;
    char *raw_ptr;

    cfg->last_error = NULL;
    cfg->path = strdup(path);
    cfg->settings_len = 0;

    fd = open(path, O_RDONLY, 0600);
    if (fd == -1) {
        cfg_set_last_error(cfg, "failed to open %s", path);
        return 1;
    }

    if (cfg_get_file_size(fd) == 0) {
        cfg_set_last_error(cfg, "failed to get file size for %s", path);
        status = 1;
        goto cfg_load_close_fd;
    }

    raw_len = lseek(fd, 0, SEEK_END);
    if (raw_len == -1) {
        cfg_set_last_error(cfg, "failed to seek end of file for %s", path);
        status = 1;
        goto cfg_load_close_fd;
    }

    raw_ptr = mmap(0, raw_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (raw_ptr == MAP_FAILED) {
        cfg_set_last_error(cfg, "failed to map file content to memory %s", path);
        status = 1;
        goto cfg_load_close_fd;
    }

    if (cfg_parse(cfg, raw_ptr, (size_t)raw_len) != 0) {
        cfg_set_last_error(cfg, "parsing failed: %s", cfg_get_last_error(cfg));
        status = 1;
    }

    munmap(raw_ptr, raw_len);

cfg_load_close_fd:
    close(fd);

    return status;
}

/**
 * @brief get a setting value
 * @param cfg pointer to the cfg_t object
 * @param identifier identifier string
 * @param value (out) address of the variable to write value data to
 * @returns 0 on success, 1 otherwise
*/
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
                    cfg_set_last_error(cfg, "huh?");
                    return 1;
                }
            }
        }
    }
    
    cfg_set_last_error(cfg, "setting doesn't exist: %s", identifier);
    return 1;
}
