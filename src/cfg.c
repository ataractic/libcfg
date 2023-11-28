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

int cfg_errno = 0; /* config error */

static cfg_t cfg_g = { /* cfg object */
    .col = 1,
    .line = 1,
    .path = NULL,
    .settings = NULL,
    .settings_len = 0
};

static const char* cfg_error_string_list[] = { /* error strings */
    [CFG_SUCCESS] = "success",
    [CFG_EMEM] = "out of memory",
    [CFG_EINVINT] = "invalid integer",
    [CFG_EINVFLOAT] = "invalid float",
    [CFG_EINVSTRING] = "invalid string",
    [CFG_EINVBOOL] = "invalid boolean",
    [CFG_EINVNULL] = "invalid value",
    [CFG_EINVID] = "invalid identifier",
    [CFG_ERANGE] = "out of range",
    [CFG_EPARSE] = "parsing failed",
    [CFG_EOPEN] = "failed to open",
    [CFG_ESIZE] = "failed to get file size",
    [CFG_ESEEK] = "failed to seek end of file",
    [CFG_EMAP] = "failed to map file content to memory",
    [CFG_ENEXIST] = "setting doesn't exist",
    [CFG_EHUH] = "huh?",
};

/**
 * @brief gets the string corresponding to the error number
 * @param errnum error number
 * @returns pointer to error string
*/
const char *cfg_strerror(int errnum) {
    return cfg_error_string_list[errnum];
}

/**
 * @brief prints the provided error string to stderr, followed by the latest libcfg error string
 * @param error_string error string
*/
void cfg_perror(const char *error_string) {
    fprintf(stderr, "%s: %s\n", error_string ? error_string : "", cfg_strerror(cfg_errno));
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
*/
void cfg_free(void) {
    cfg_setting_t* current;

    for (size_t i = 0; i < cfg_g.settings_len; ++i) {
        current = cfg_g.settings[i];

        if (current->type == CFG_STYPE_STRING) {
            free(current->string);
        }

        free(current->identifier);
        free(current);
    }

    if (cfg_g.settings_len != 0) {
        free(cfg_g.settings);
    }

    if (cfg_g.path != NULL) {
        free(cfg_g.path);
    }
}

/**
 * @brief outputs the current loaded configuration to the console
*/
void cfg_dump(void) {
    cfg_setting_t* current;

    for (size_t i = 0; i < cfg_g.settings_len; ++i) {
        current = cfg_g.settings[i];

        switch (current->type) {
            case CFG_STYPE_STRING: {
                printf("%s=\"%s\"\n", current->identifier, current->string);
                break;
            }
            case CFG_STYPE_INT: {
                printf("%s=%lld\n", current->identifier, current->integer);
                break;
            }
            case CFG_STYPE_BOOL: {
                printf("%s=%s\n", current->identifier, current->boolean ? "true" : "false");
                break;
            }
            case CFG_STYPE_FLOAT: {
                printf("%s=%Lf\n", current->identifier, current->floating);
                break;
            }
            case CFG_STYPE_UNKNOWN: {
                break;
            }
        }
    }
}

/**
 * @brief adds a setting to the configuration
 * @param setting pointer to the setting object
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_setting(cfg_setting_t* setting) {
    void* tmp;

    tmp = realloc(cfg_g.settings_len == 0 ? NULL : cfg_g.settings, sizeof(cfg_setting_t*) * (cfg_g.settings_len + 1));

    if (tmp == NULL) {
        cfg_errno = CFG_EMEM;
        return 1;
    }
    
    cfg_g.settings = tmp;
    cfg_g.settings_len += 1;
    cfg_g.settings[cfg_g.settings_len - 1] = setting;

    return 0;
}

/**
 * @brief adds an string setting to the configuration object
 * @param str pointer to the string value
 * @param str_len length of string value
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_string_setting(const char* str, size_t str_len, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = CFG_STYPE_STRING;
    setting->identifier = strndup(id, id_len);
    setting->string = strndup(str, str_len);

    if (cfg_add_setting(setting) != 0) {
        free(setting->string);
        free(setting->identifier);
        free(setting);
        return 1;
    }

    return 0;
}

/**
 * @brief adds an boolean value setting to the configuration object
 * @param b value to add
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_boolean_setting(bool b, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = CFG_STYPE_BOOL;
    setting->identifier = strndup(id, id_len);
    setting->boolean = b;

    if (cfg_add_setting(setting) != 0) {
        free(setting->identifier);
        free(setting);
        return 1;
    }

    return 0;
}

/**
 * @brief adds an floating point number setting to the configuration object
 * @param value value to add
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_floating_setting(long double value, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = CFG_STYPE_FLOAT;
    setting->identifier = strndup(id, id_len);
    setting->floating = value;

    if (cfg_add_setting(setting) != 0) {
        free(setting->identifier);
        free(setting);
        return 1;
    }

    return 0;
}

/**
 * @brief adds an integer number setting to the configuration object
 * @param value value to add
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_add_integer_setting(long long value, const char* id, size_t id_len) {
    cfg_setting_t* setting = malloc(sizeof(cfg_setting_t));

    setting->type = CFG_STYPE_INT;
    setting->identifier = strndup(id, id_len);
    setting->integer = value;

    if (cfg_add_setting(setting) != 0) {
        free(setting->identifier);
        free(setting);
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
 * @param str pointer to the serialized value token
 * @param len length of the serialized value token
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_parse_integer(const char* str, size_t len, const char* id, size_t id_len) {
    int status = 0;
    char *endptr;
    long long result;
    char* copy;

    errno = 0;
    copy = strndup(str, len); /* handle case where the value is the last thing in the file */
    if (copy == NULL && errno == ENOMEM) {
        cfg_errno = CFG_EMEM;
        return 1;
    }

    errno = 0;
    result = strtoll(copy, &endptr, 10);
    if (endptr == copy) {
        cfg_errno = CFG_EINVINT;
        status = 1;
        goto cfg_parse_integer_free;
    }

    if (errno == ERANGE || errno == EINVAL) {
        cfg_errno = CFG_ERANGE;
        status = 1;
        goto cfg_parse_integer_free;
    }

    if (cfg_add_integer_setting(result, id, id_len) != 0) {
        /* report another error here? */
        status = 1;
    }

cfg_parse_integer_free:
    free(copy);

    return status;
}

/**
 * @brief parses a floating point number from the buffer
 * @param str pointer to the serialized value token
 * @param len length of the serialized value token
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_parse_floating(const char* str, size_t len, const char* id, size_t id_len) {
    int status = 0;
    char *endptr;
    long double result;
    char* copy;

    errno = 0;
    copy = strndup(str, len); /* handle case where the value is the last thing in the file */
    if (copy == NULL && errno == ENOMEM) {
        cfg_errno = CFG_EMEM;
        return 1;
    }

    errno = 0;
    result = strtold(copy, &endptr);
    if (endptr == copy) {
        cfg_errno = CFG_EINVFLOAT;
        status = 1;
        goto cfg_parse_floating_free;
    }

    if (errno == ERANGE || errno == EINVAL) {
        cfg_errno = CFG_ERANGE;
        status = 1;
        goto cfg_parse_floating_free;
    }

    if (cfg_add_floating_setting(result, id, id_len) != 0) {
        /* report another error here? */
        status = 1;
    }

cfg_parse_floating_free:
    free(copy);

    return status;
}

/**
 * @brief parses a string from the buffer
 * @param str pointer to the serialized value token
 * @param len length of the serialized value token
 * @param id pointer to the identifier token
 * @param id_len length of the identifier token
 * @returns 0 on success, 1 otherwise
*/
static int cfg_parse_string(const char* str, size_t len, const char* id, size_t id_len) {
    if (len < 2 || str[0] != '\"' || str[len - 1] != '\"') {
        cfg_errno = CFG_EINVSTRING;
        return 1;
    }

    if (cfg_add_string_setting(&str[1], len - 2, id, id_len) != 0) {
        /* report another error here? */
        return 1;
    }

    return 0;
}

/**
 * @brief parses the serialized configuration buffer
 * @param str pointer to the buffer containing the serialized configuration
 * @param len length of the buffer
 * @returns 0 on success, 1 otherwise with cfg_errno set
*/
int cfg_parse(const char* str, size_t len) {
    size_t c1 = 0;
    size_t c2 = 0;
    size_t id_pos = 0;
    size_t id_len = 0;
    size_t value_pos = 0;
    size_t value_len = 0;

    while (c2 < len) {
        switch (str[c2]) {
            /* forward the cursor until something meaningful */
            case '\r':
            case '\t':
            case ' ': {
                c2 += 1;
                cfg_g.col += 1;
                break;
            }
            /* end of line expect an identifier */
            case '\n': {
                c2 += 1;
                
                cfg_g.line += 1;
                cfg_g.col = 1;
                break;
            }
            /* forward the cursor to the end of the line */
            case '#': {
                while (c2 < len && str[c2] != '\n') {
                    c2 += 1;
                    cfg_g.col += 1;
                }
                break;
            }
            /* we have an identifier */
            default: {
                c1 = c2;
                /* get the position and length of the identifier */
                while (c2 < len && str[c2] != '=') {
                    c2 += 1;
                    cfg_g.col += 1;
                }
                id_pos = c1;
                id_len = c2 - c1;


                if (id_len == 0) {
                    cfg_errno = CFG_EINVID;
                    return 1;
                }

                /* trim whitespaces at the end of the identifier */
                for (size_t i = 1; cfg_is_whitespace(str[c2 - i]); i++) {
                    id_len -= 1;
                }

                /* check for key validity */
                if (!cfg_is_identifier_valid(&str[id_pos], id_len)) {
                    cfg_errno = CFG_EINVID;
                    return 1;
                }

                /* skip the assignment operator */
                if (c2 + 1 < len && str[c2] == '=') {
                    c2 += 1;
                    cfg_g.col += 1;
                }

                /* forward to the value */
                while (c2 < len && cfg_is_whitespace(str[c2])) {
                    c2 += 1;
                    cfg_g.col += 1;
                }
                c1 = c2;

                /* get the position and length of the value until end of line */
                while (c2 < len && str[c2] != '\n' && str[c2] != '#') {
                    c2 += 1;
                    cfg_g.col += 1;
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
                            cfg_errno = CFG_ENEXIST;
                            return 1;
                        }
                        if (memchr(&str[value_pos], '.', value_len) != NULL) {
                            if (cfg_parse_floating(&str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                                return 1;
                            }
                        } else {
                            if (cfg_parse_integer(&str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                                return 1;
                            }
                        }              
                        break;
                    }
                    /* the value should be a string */
                    case '\"': {
                        if (cfg_parse_string(&str[value_pos], value_len, &str[id_pos], id_len) != 0) {
                            return 1;
                        }                        
                        break;
                    }
                    /* the value should be a bool */
                    case 'f':
                    case 't': {
                        if (strncmp(&str[value_pos], "true", value_len) == 0) {
                            if (cfg_add_boolean_setting(1, &str[id_pos], id_len) != 0) {
                                return 1;
                            }
                        } else if (strncmp(&str[value_pos], "false", value_len) == 0) {
                            if (cfg_add_boolean_setting(0, &str[id_pos], id_len) != 0) {
                                return 1;
                            }
                        } else {
                            cfg_errno = CFG_EINVBOOL;
                            return 1;
                        }
                        break;
                    }
                    default: {
                        cfg_errno = CFG_EINVNULL;
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
    return (size_t)(s.st_size);
}

/**
 * @brief loads a supported config file into the program
 * @param path path to the config file
 * @returns 0 on success, 1 otherwise with cfg_errno set
*/
int cfg_load(const char* path) {
    int status = 0;
    int fd;
    off_t raw_len;
    char *raw_ptr;

    cfg_g.path = strdup(path);

    fd = open(path, O_RDONLY, 0600);
    if (fd == -1) {
        cfg_errno = CFG_EOPEN;
        return 1;
    }

    if (cfg_get_file_size(fd) == 0) {
        cfg_errno = CFG_ESIZE;
        status = 1;
        goto cfg_load_close_fd;
    }

    raw_len = lseek(fd, 0, SEEK_END);
    if (raw_len == -1) {
        cfg_errno = CFG_ESEEK;
        status = 1;
        goto cfg_load_close_fd;
    }

    raw_ptr = mmap(0, (size_t)raw_len, PROT_READ, MAP_PRIVATE, fd, 0);
    if (raw_ptr == MAP_FAILED) {
        cfg_errno = CFG_EMAP;
        status = 1;
        goto cfg_load_close_fd;
    }

    if (cfg_parse(raw_ptr, (size_t)raw_len) != 0) {
        status = 1;
    }

    munmap(raw_ptr, (size_t)raw_len);

cfg_load_close_fd:
    close(fd);

    return status;
}

/**
 * @brief get a setting value
 * @param identifier identifier string
 * @param value (out) address of the variable to write value data to
 * @returns 0 on success, 1 otherwise
*/
int cfg_get_setting(const char* identifier, void* value) {
    for (size_t i = 0; i < cfg_g.settings_len; i++) {
        if (strcmp(identifier, cfg_g.settings[i]->identifier) == 0) {
            switch (cfg_g.settings[i]->type) {
                case CFG_STYPE_BOOL: {
                    *(bool*)value = cfg_g.settings[i]->boolean;
                    return 0;
                }
                case CFG_STYPE_STRING: {
                    *(char**)value = cfg_g.settings[i]->string;
                    return 0;
                }
                case CFG_STYPE_INT: {
                    *(long long*)value = cfg_g.settings[i]->integer;
                    return 0;
                }
                case CFG_STYPE_FLOAT: {
                    *(long double*)value = cfg_g.settings[i]->floating;
                    return 0;
                }
                case CFG_STYPE_UNKNOWN: {
                    cfg_errno = CFG_EHUH;
                    return 1;
                }
            }
        }
    }
    
    cfg_errno = CFG_ENEXIST;
    return 1;
}

/**
 * @brief get the line at wich the parser stopped
 * @returns the line number
*/
size_t cfg_get_error_line(void) {
    return cfg_g.line;
}

/**
 * @brief get the column at wich the parser stopped
 * @returns the column number
*/
size_t cfg_get_error_col(void) {
    return cfg_g.col;
}

/**
 * @brief returns the path of the loaded configuration file
 * @returns pointer to the path string or NULL if parsed from a buffer
*/
const char* cfg_get_path(void) {
    return cfg_g.path;
}

/**
 * @brief get a setting type
 * @param identifier setting identifier
 * @returns type of the corresponding setting
*/
enum cfg_setting_type_e cfg_get_setting_type(const char* identifier) {
    for (size_t i = 0; i < cfg_g.settings_len; i++) {
        if (strcmp(identifier, cfg_g.settings[i]->identifier) == 0) {
            return cfg_g.settings[i]->type;
        }
    }

    return CFG_STYPE_UNKNOWN;
}
