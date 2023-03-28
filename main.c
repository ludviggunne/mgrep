
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILES 64
#define MAX_TERMS 32

#define ERREXIT(...) do { fprintf(stderr, __VA_ARGS__); exit(1); } while(0)

struct {

    const char *paths[MAX_FILES];
    size_t      npaths;
    const char *terms[MAX_TERMS];
    size_t      nterms;

    int caseinstv;
    int all;
} config;

void parse_args(int argc, const char *argv[]);

int main(int argc, const char *argv[]) {

    parse_args(argc, argv);

    printf("Files (%d):\n", config.npaths);
    for (int i = 0; i < config.npaths; i++) {

        printf("%s\n", config.paths[i]);
    }

    printf("Terms (%d):\n", config.nterms);
    for (int i = 0; i < config.nterms; i++) {

        printf("%s\n", config.terms[i]);
    }
}

void parse_args(int argc, const char *argv[])
{

    // Usage:
    //     mgrep [options] <files ...> <terms ...>
    // Options:
    //     -c    Case insensitive NOT IMPL
    //     -a    Line must contain all terms

    enum parse_state_t { NONE, FILE, TERM }; 
    enum parse_state_t parse_state = NONE;
    config.npaths = 0;
    config.nterms = 0;

    for (int argi = 1; argi < argc; argi++)
    {

        const char *arg = argv[argi];

        if (arg[0] == '-') {

            if (strlen(arg) != 2) {

                ERREXIT("ERROR: Invalid option: \"%s\"\n", arg);
            }

            switch (arg[1]) {

                case 'f': 
                    parse_state = FILE;
                    break;

                case 't':
                    parse_state = TERM;
                    break; 

                case 'c':
                    config.caseinstv = 1;
                    break;
                
                case 'a':
                    config.all = 1;
                    break;
                
                default:
                    ERREXIT("ERROR: Invalid option: \"%s\"\n", arg);
            }
        } else {

            switch (parse_state) {

                case FILE:
                    if (config.npaths == MAX_FILES) {

                        ERREXIT("ERROR: Too many file paths specified (max = %d)\n", MAX_FILES);
                    }
                    config.paths[config.npaths] = arg;
                    config.npaths++;
                    break;

                case TERM:
                    if (config.nterms == MAX_TERMS) {

                        ERREXIT("ERROR: Too many terms specified (max = %d)\n", MAX_TERMS);
                    }
                    config.terms[config.nterms] = arg;
                    config.nterms++;
                    break;

                case NONE:
                    ERREXIT("ERROR: Unexpected token: \"%s\". Use -f or -t to specify files/terms\n", arg);
            }
        }
    }

    if (config.npaths == 0) {

        ERREXIT("ERROR: No files specified\n");
    }

    if (config.nterms == 0) {

        ERREXIT("ERROR: No terms specified\n");
    }
}

void usage() {

    printf("Usage:\n    mgrep -f <file> <...> -t <term> <...> [options]\n");
}