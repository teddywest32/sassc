#ifdef _MSC_VER
#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS 1
#endif
#ifndef _CRT_NONSTDC_NO_WARNINGS
#define _CRT_NONSTDC_NO_WARNINGS 1
#endif
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <sass.h>
#include "sassc_version.h"

#define BUFSIZE 512
#ifdef _WIN32
#define PATH_SEP ';'
#else
#define PATH_SEP ':'
#endif

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <windows.h>
int get_argv_utf8(int* argc_ptr, char*** argv_ptr) {
  int argc;
  char** argv;
  wchar_t** argv_utf16 = CommandLineToArgvW(GetCommandLineW(), &argc);
  int i;
  int offset = (argc + 1) * sizeof(char*);
  int size = offset;
  for (i = 0; i < argc; i++)
    size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, 0, 0, 0, 0);
  argv = malloc(size);
  for (i = 0; i < argc; i++) {
    argv[i] = (char*) argv + offset;
    offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1,
      argv[i], size-offset, 0, 0);
  }
  *argc_ptr = argc;
  *argv_ptr = argv;
  return 0;
}
#endif

int output(int error_status, const char* error_message, const char* output_string, const char* outfile) {
    if (error_status) {
        if (error_message) {
            fprintf(stderr, "%s", error_message);
        } else {
            fprintf(stderr, "An error occured; no error message available.\n");
        }
        return 1;
    } else if (output_string) {
        if(outfile) {
            FILE* fp = fopen(outfile, "wb");
            if(!fp) {
                perror("Error opening output file");
                return 1;
            }
            if(fprintf(fp, "%s", output_string) < 0) {
                perror("Error writing to output file");
                fclose(fp);
                return 1;
            }
            fclose(fp);
        }
        else {
            #ifdef _WIN32
              setmode(fileno(stdout), O_BINARY);
            #endif
            printf("%s", output_string);
        }
        return 0;
    } else {
        fprintf(stderr, "Unknown internal error.\n");
        return 2;
    }
}

int compile_stdin(struct Sass_Options* options, char* outfile) {
    int ret;
    struct Sass_Data_Context* ctx;
    char buffer[BUFSIZE];
    size_t size = 1;
    char *source_string = malloc(sizeof(char) * BUFSIZE);

    if(source_string == NULL) {
        perror("Allocation failed");
        exit(1);
    }

    source_string[0] = '\0';

    while(fgets(buffer, BUFSIZE, stdin)) {
        char *old = source_string;
        size += strlen(buffer);
        source_string = realloc(source_string, size);
        if(source_string == NULL) {
            perror("Reallocation failed");
            free(old);
            exit(2);
        }
        strcat(source_string, buffer);
    }

    if(ferror(stdin)) {
        free(source_string);
        perror("Error reading standard input");
        exit(2);
    }

    ctx = sass_make_data_context(source_string);
    struct Sass_Context* ctx_out = sass_data_context_get_context(ctx);
    sass_data_context_set_options(ctx, options);
    sass_compile_data_context(ctx);
    ret = output(
        sass_context_get_error_status(ctx_out),
        sass_context_get_error_message(ctx_out),
        sass_context_get_output_string(ctx_out),
        outfile
    );
    sass_delete_data_context(ctx);
    return ret;
}

int compile_file(struct Sass_Options* options, char* input_path, char* outfile) {
    int ret;
    struct Sass_File_Context* ctx = sass_make_file_context(input_path);
    struct Sass_Context* ctx_out = sass_file_context_get_context(ctx);
    if (outfile) sass_option_set_output_path(options, outfile);
    sass_option_set_input_path(options, input_path);
    sass_file_context_set_options(ctx, options);

    sass_compile_file_context(ctx);

    ret = output(
        sass_context_get_error_status(ctx_out),
        sass_context_get_error_message(ctx_out),
        sass_context_get_output_string(ctx_out),
        outfile
    );

    if (ret == 0 && sass_option_get_source_map_file(options)) {
        ret = output(
            sass_context_get_error_status(ctx_out),
            sass_context_get_error_message(ctx_out),
            sass_context_get_source_map_string(ctx_out),
            sass_option_get_source_map_file(options)
        );
    }

    sass_delete_file_context(ctx);
    return ret;
}

struct
{
    char* style_string;
    int output_style;
} style_option_strings[] = {
    { "compressed", SASS_STYLE_COMPRESSED },
    { "compact", SASS_STYLE_COMPACT },
    { "expanded", SASS_STYLE_EXPANDED },
    { "nested", SASS_STYLE_NESTED }
};

#define NUM_STYLE_OPTION_STRINGS \
    sizeof(style_option_strings) / sizeof(style_option_strings[0])

void print_version() {
    printf("sassc: %s\n", SASSC_VERSION);
    printf("libsass: %s\n", libsass_version());
    printf("sass2scss: %s\n", sass2scss_version());
    printf("sass: %s\n", libsass_language_version());
}

void print_usage(char* argv0) {
    int i;
    printf("Usage: %s [options] [INPUT] [OUTPUT]\n\n", argv0);
    printf("Options:\n");
    printf("   -s, --stdin             Read input from standard input instead of an input file.\n");
    printf("   -t, --style NAME        Output style. Can be:");
    for(i = NUM_STYLE_OPTION_STRINGS - 1; i >= 0; i--) {
        printf(" %s", style_option_strings[i].style_string);
        printf(i == 0 ? ".\n" : ",");
    }
    printf("   -l, --line-numbers      Emit comments showing original line numbers.\n");
    printf("       --line-comments\n");
    printf("   -I, --load-path PATH    Set Sass import path.\n");
    printf("   -P, --plugin-path PATH  Set path to autoload plugins.\n");
    printf("   -m, --sourcemap         Emit source map.\n");
    printf("   -M, --omit-map-comment  Omits the source map url comment.\n");
    printf("   -p, --precision         Set the precision for numbers.\n");
    printf("   -v, --version           Display compiled versions.\n");
    printf("   -h, --help              Display this help message.\n");
    printf("\n");
}

void invalid_usage(char* argv0) {
    fprintf(stderr, "See '%s -h'\n", argv0);
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
#ifdef _MSC_VER
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior( 0, _WRITE_ABORT_MSG);
#endif
#ifdef _WIN32
    get_argv_utf8(&argc, &argv);
#endif
    char *outfile = 0;
    int from_stdin = 0;
    bool generate_source_map = false;
    struct Sass_Options* options = sass_make_options();
    sass_option_set_output_style(options, SASS_STYLE_NESTED);
    sass_option_set_precision(options, 5);

    int c;
    size_t i;
    int long_index = 0;
    static struct option long_options[] =
    {
        { "stdin",              no_argument,       0, 's' },
        { "load-path",          required_argument, 0, 'I' },
        { "plugin-path",        required_argument, 0, 'P' },
        { "style",              required_argument, 0, 't' },
        { "line-numbers",       no_argument,       0, 'l' },
        { "line-comments",      no_argument,       0, 'l' },
        { "sourcemap",          no_argument,       0, 'm' },
        { "omit-map-comment",   no_argument,       0, 'M' },
        { "precision",          required_argument, 0, 'p' },
        { "version",            no_argument,       0, 'v' },
        { "help",               no_argument,       0, 'h' },
        { NULL,                 0,                 NULL, 0}
    };
    while ((c = getopt_long(argc, argv, "vhslmMp:t:I:P:", long_options, &long_index)) != -1) {
        switch (c) {
        case 's':
            from_stdin = 1;
            break;
        case 'I':
            sass_option_push_include_path(options, strdup(optarg));
            break;
        case 'P':
            sass_option_push_plugin_path(options, strdup(optarg));
            break;
        case 't':
            for(i = 0; i < NUM_STYLE_OPTION_STRINGS; ++i) {
                if(strcmp(optarg, style_option_strings[i].style_string) == 0) {
                    sass_option_set_output_style(options, style_option_strings[i].output_style);
                    break;
                }
            }
            if(i == NUM_STYLE_OPTION_STRINGS) {
                fprintf(stderr, "Invalid argument for -t flag: '%s'. Allowed arguments are:", optarg);
                for(i = 0; i < NUM_STYLE_OPTION_STRINGS; ++i) {
                    fprintf(stderr, " %s", style_option_strings[i].style_string);
                }
                fprintf(stderr, "\n");
                invalid_usage(argv[0]);
            }
            break;
        case 'l':
            sass_option_set_source_comments(options, true);
            break;
        case 'm':
            generate_source_map = true;
            break;
        case 'M':
            sass_option_set_omit_source_map_url(options, true);
            break;
        case 'p':
            sass_option_set_precision(options, atoi(optarg)); // TODO: make this more robust
            if (sass_option_get_precision(options) < 0) sass_option_set_precision(options, 5);
            break;
        case 'v':
            print_version();
            return 0;
        case 'h':
            print_usage(argv[0]);
            return 0;
        case '?':
            /* Unrecognized flag or missing an expected value */
            /* getopt should produce it's own error message for this case */
            invalid_usage(argv[0]);
        default:
            fprintf(stderr, "Unknown error while processing arguments\n");
            return 2;
        }
    }

    if(optind < argc - 2) {
        fprintf(stderr, "Error: Too many arguments.\n");
        invalid_usage(argv[0]);
    }

    int result;
    if(optind < argc && strcmp(argv[optind], "-") != 0 && !from_stdin) {
        if (optind + 1 < argc) {
            outfile = argv[optind + 1];
        }
        if (generate_source_map && outfile) {
            const char* extension = ".map";
            char* source_map_file  = calloc(strlen(outfile) + strlen(extension) + 1, sizeof(char));
            strcpy(source_map_file, outfile);
            strcat(source_map_file, extension);
            sass_option_set_source_map_file(options, source_map_file);
        }
        result = compile_file(options, argv[optind], outfile);
    } else {
        if (optind < argc) {
            outfile = argv[optind];
        }
        result = compile_stdin(options, outfile);
    }

    return result;
}
