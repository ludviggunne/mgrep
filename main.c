
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define MAX_THREADS 4
#define MAX_FILES   64
#define MAX_TERMS   32

#define ERREXIT(...) do { fprintf(stderr, __VA_ARGS__); exit(1); } while(0)

// ANSI escape sequences for text color
#define ESC(n) "\033[" #n "m"
#define RESET ESC(0)

#define USAGE \
"Usage:\n"\
"    %s -f <file>... -t <term>... [options]\n"\
"        file            file(s) to be searched (stdin used if no path is specified)\n"\
"        term            terms(s) to be queried\n"\
"    A maximum of %d files and %d terms may be specified.\n"\
"    Options:\n"\
"        -c              Case insensitive search\n"\
"        -a              Line must contain all specified terms (not implemented)\n"\
"        -m              Enable multithreading (not implemented)\n"
#define USAGE_PARAMS config.prgm_name, MAX_FILES, MAX_TERMS

const char *COLORS[] = {

    ESC(91),
    ESC(92),
    ESC(93),
    ESC(94),
    ESC(95),
    ESC(96)
};
const size_t NCOLORS = sizeof(COLORS) / sizeof(COLORS[0]); 

enum options {

    CASE_INSENSITIVE,
    MATCH_ALL,
    USE_THREADS,
    OPTION_COUNT
};
typedef struct {

    const char *prgm_name;

    const char *paths[MAX_FILES];
    size_t      npaths;
    const char *terms[MAX_TERMS];
    size_t      term_lens[MAX_TERMS];
    size_t      nterms;

    int         options[OPTION_COUNT];
} config_t;
config_t config;
void parse_args(int argc, const char *argv[]);



typedef struct {

    char *data;
    size_t length;
    size_t capacity;
} buffer_t;
int  buffer_init(buffer_t *buf, size_t capacity);
int  buffer_append(buffer_t *buf, const char *data);
int  buffer_append_range(buffer_t *buf, const char *begin, const char *end);
void buffer_reset(buffer_t *buf);
void buffer_free(buffer_t *buf);
int  buffer_realloc(buffer_t *buf, size_t capacity);
int  buffer_null_terminate(buffer_t *buf);

enum search_status {

    OK,
    FILE_ERR,
    MEM_ERR
};
typedef struct {

    // path == NULL -> use stdout
    const char        *path;
    buffer_t           result;
    enum search_status status;
} thread_data_t;

void *search_file(thread_data_t *data);

int main(int argc, const char *argv[]) {

    parse_args(argc, argv);

    if (config.options[USE_THREADS] && config.npaths > 1) {

        ERREXIT("ERROR: Sorry, multi-threading is not supported yet!\n");
    } else {

        thread_data_t data;
        data.status = OK;
        if (!buffer_init(&data.result, 100)) {

            ERREXIT("ERROR: Unable to create buffer\n");
        }

        // Iterate over files
        for (int i = 0; i < config.npaths; i++) {

            data.path = config.paths[i];
            buffer_reset(&data.result); 

            search_file(&data);

            const char *fname = data.path ? data.path : "stdout";

            if (data.status == FILE_ERR) {
                
                printf("ERROR: Could not open/read file %s\n", fname);
            }

            if (data.status == MEM_ERR) {

                printf("ERROR: Out of memory while reading file %s\n", fname);
            }

            if (data.result.length > 1) {

                printf("Results from file %s:\n%s\n", fname, data.result.data);                
            }            
        }

        buffer_free(&data.result);
    }
}



void parse_args(int argc, const char *argv[])
{
    enum parse_state_t { S_NONE, S_FILE, S_TERM }; 
    enum parse_state_t parse_state = S_NONE;
    
    memset(&config, 0, sizeof(config));

    config.prgm_name = argv[0];

    for (int argi = 1; argi < argc; argi++)
    {

        const char *arg = argv[argi];

        if (arg[0] == '-') {

            if (strlen(arg) != 2) {

                ERREXIT("ERROR: Invalid option: \"%s\"\n" USAGE, arg, USAGE_PARAMS);
            }

            switch (arg[1]) {

                case 'f': 
                    parse_state = S_FILE;
                    break;

                case 't':
                    parse_state = S_TERM;
                    break; 

                case 'c':
                    config.options[CASE_INSENSITIVE] = 1;
                    break;
                
                case 'a':
                    config.options[MATCH_ALL] = 1;
                    break;
                
                case 'm':
                    config.options[USE_THREADS] = 1;
                    break;

                default:
                    ERREXIT("ERROR: Invalid option: \"%s\"\n" USAGE, arg, USAGE_PARAMS);
            }
        } else {

            switch (parse_state) {

                case S_FILE:
                    if (config.npaths == MAX_FILES) {

                        ERREXIT("ERROR: Too many file paths specified (max = %d)\n", MAX_FILES);
                    }
                    config.paths[config.npaths] = arg;
                    config.npaths++;
                    break;

                case S_TERM:
                    if (config.nterms == MAX_TERMS) {

                        ERREXIT("ERROR: Too many terms specified (max = %d)\n", MAX_TERMS);
                    }
                    config.terms[config.nterms] = arg;
                    config.term_lens[config.nterms] = strlen(arg);
                    config.nterms++;
                    break;

                case S_NONE:
                    ERREXIT("ERROR: Unexpected token: \"%s\". Use -f or -t to specify files/terms\n" USAGE, arg, USAGE_PARAMS);
            }
        }
    }

    if (config.npaths == 0) {

        // We set a single path to NULL to signal to search_file that we want to use stdout
        config.paths[0] = NULL;
        config.npaths++;
    }

    if (config.nterms == 0) {

        ERREXIT("ERROR: No terms specified\n" USAGE, USAGE_PARAMS);
    }
}

#define CHECK(x) if (!x) { data->status = MEM_ERR; return NULL; }
void *search_file(thread_data_t *data)
{

    FILE *file;    
    if (data->path == NULL) {

        file = stdin;
    } else {

        file = fopen(data->path, "r");

        if (file == NULL) {

            data->status = FILE_ERR;
            return NULL;
        }
    }


    char   *line = NULL;
    size_t  buf_sz = 0;
    long    line_len;

    while ((line_len = getline(&line, &buf_sz, file)) != -1) {


        size_t offset = 0;
        for (size_t col = 0; col < line_len; col++) {
            
            for (size_t term_id = 0; term_id < config.nterms; term_id++) {
            
                const char  *term = config.terms[term_id];
                const size_t term_len = config.term_lens[term_id];

                if ((config.options[CASE_INSENSITIVE] ? 
                    strncasecmp(&line[col], term, term_len) :
                    strncmp(&line[col], term, term_len)) == 0)    
                {

                    CHECK(buffer_append_range(&data->result, &line[offset], &line[col]));
                    CHECK(buffer_append(&data->result, COLORS[term_id % NCOLORS]));
                    CHECK(buffer_append_range(&data->result, &line[col], &line[col + term_len]));
                    CHECK(buffer_append(&data->result, RESET));
                    col += config.term_lens[term_id];
                    offset = col;
                }
            }

        }

        if (offset > 0) {

            CHECK(buffer_append(&data->result, &line[offset]));
        }
    }

    CHECK(buffer_null_terminate(&data->result));

    if (line)
        free(line);

    if (file != stdin)
        fclose(file);

    return NULL;
}

int buffer_init(buffer_t *buf, size_t capacity)
{

    buf->data = malloc(capacity);
    if (buf->data == NULL) {

        return 0;
    }

    buf->capacity = capacity;
    buf->length = 0;

    return 1;
}

int  buffer_append(buffer_t *buf, const char *data)
{

    while (*data != '\0') {

        if (buf->length == buf->capacity) {

            if (!buffer_realloc(buf, 2 * buf->capacity)) {

                return 0;
            }
        }

        buf->data[buf->length] = *data;
        buf->length++;
        data++;
    }

    return 1;
}

int  buffer_append_range(buffer_t *buf, const char *begin, const char *end)
{

    while (begin != end) {

        if (buf->length == buf->capacity) {

            if (!buffer_realloc(buf, 2 * buf->capacity)) {

                return 0;
            }
        }

        buf->data[buf->length] = *begin;
        buf->length++;
        begin++;
    }

    return 1;
}

void buffer_reset(buffer_t *buf)
{

    buf->length = 0;
}

void buffer_free(buffer_t *buf)
{

    buf->length = 0;
    buf->capacity = 0;
    free(buf->data);
    buf->data = NULL;
}

int  buffer_realloc(buffer_t *buf, size_t capacity)
{

    char *new_data = malloc(capacity);
    if (new_data == NULL) {

        return 0;
    }

    memcpy(new_data, buf->data, buf->length);
    free(buf->data);
    buf->data = new_data;
    buf->capacity = capacity;

    return 1;
}

int  buffer_null_terminate(buffer_t *buf)
{

    char nl[] = { '\0', '\0' };
    return buffer_append_range(buf, &nl[0], &nl[1]);
}
