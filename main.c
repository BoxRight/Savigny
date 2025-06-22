/**
 * main.c - Modified to include context support
 * 
 * Main program for the Kelsen schema transpiler with legal context integration
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "schema_types.h"
#include "config_validator.h"
#include "custom_tokenizer.h"
#include "context_manager.h" // New inclusion for context support

/* Function prototype from the Bison parser */
extern Schema* parse_schema(FILE* input);

/**
 * Print usage information
 */
void print_usage(const char* program_name) {
    printf("Usage: %s [options] input_file [output_file]\n\n", program_name);
    printf("Options:\n");
    printf("  -h, --help       Display this help message\n");
    printf("  -v, --verbose    Enable verbose output\n");
    printf("  -c, --config FILE  Specify configuration file (default: schema_config.json)\n");
    printf("  -x, --context FILE Specify legal context file\n");  // New option
    printf("\n");
    printf("If output_file is not specified, output is written to stdout.\n");
}

/**
 * Main function
 */
int main(int argc, char** argv) {
    /* Default options */
    int verbose = 0;
    char* input_filename = NULL;
    char* output_filename = NULL;
    char* config_filename = "schema_config.json";
    char* context_filename = NULL;  // Default: no context file
    
    /* Parse command-line arguments */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return EXIT_SUCCESS;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            verbose = 1;
        } else if (strcmp(argv[i], "-c") == 0 || strcmp(argv[i], "--config") == 0) {
            if (i + 1 < argc) {
                config_filename = argv[++i];
            } else {
                fprintf(stderr, "Error: Missing argument for %s\n", argv[i]);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else if (strcmp(argv[i], "-x") == 0 || strcmp(argv[i], "--context") == 0) {
            if (i + 1 < argc) {
                context_filename = argv[++i];
            } else {
                fprintf(stderr, "Error: Missing argument for %s\n", argv[i]);
                print_usage(argv[0]);
                return EXIT_FAILURE;
            }
        } else if (input_filename == NULL) {
            input_filename = argv[i];
        } else if (output_filename == NULL) {
            output_filename = argv[i];
        } else {
            fprintf(stderr, "Error: Too many arguments\n");
            print_usage(argv[0]);
            return EXIT_FAILURE;
        }
    }
    
    /* Check if input file is specified */
    if (input_filename == NULL) {
        fprintf(stderr, "Error: No input file specified\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }
    
    /* Initialize configuration validator */
    if (!config_init(config_filename)) {
        fprintf(stderr, "Error: Failed to load configuration from %s\n", config_filename);
        return EXIT_FAILURE;
    }
    
    /* Initialize context manager if a context file was specified */
    if (context_filename != NULL) {
        if (!legal_context_init(context_filename)) {
            fprintf(stderr, "Error: Failed to load legal context from %s\n", context_filename);
            config_cleanup();
            return EXIT_FAILURE;
        }
        
        if (verbose) {
            printf("Legal context loaded from %s\n", context_filename);
        }
    }
    
    /* Open input file */
    FILE* input_file = fopen(input_filename, "r");
    if (input_file == NULL) {
        fprintf(stderr, "Error: Failed to open input file %s\n", input_filename);
        if (context_filename != NULL) {
            legal_context_cleanup();
        }
        config_cleanup();
        return EXIT_FAILURE;
    }
    
    if (verbose) {
        printf("Parsing schema from %s...\n", input_filename);
    }
    
    /* Parse schema */
    Schema* schema = parse_schema(input_file);
    
    /* Close input file */
    fclose(input_file);
    
    /* Check if parsing succeeded */
    if (schema == NULL) {
        fprintf(stderr, "Error: Failed to parse schema\n");
        if (context_filename != NULL) {
            legal_context_cleanup();
        }
        config_cleanup();
        return EXIT_FAILURE;
    }
    
    if (verbose) {
        printf("Successfully parsed schema\n");
        printf("Generating Kelsen code...\n");
    }

    /* Generate Kelsen code with context if available */
    char* kelsen_code;
    if (context_filename != NULL) {
        kelsen_code = generate_kelsen_code_with_context(schema);
    } else {
        kelsen_code = generate_kelsen_code(schema);
    }

    if (kelsen_code == NULL) {
        fprintf(stderr, "Error: Failed to generate Kelsen code\n");
        if (context_filename != NULL) {
            legal_context_cleanup();
        }
        free_schema(schema);
        config_cleanup();
        return EXIT_FAILURE;
    }

    /* Output Kelsen code */
    if (output_filename != NULL) {
        FILE* output_file = fopen(output_filename, "w");
        if (output_file == NULL) {
            fprintf(stderr, "Error: Failed to open output file %s\n", output_filename);
            free(kelsen_code);
            if (context_filename != NULL) {
                legal_context_cleanup();
            }
            free_schema(schema);
            config_cleanup();
            return EXIT_FAILURE;
        }
        
        fputs(kelsen_code, output_file);
        fclose(output_file);
        
        if (verbose) {
            printf("Kelsen code written to %s\n", output_filename);
        }
        
        /* Execute the Kelsen compiler on the output file */
        char command[512];
        snprintf(command, sizeof(command), "kelsen -e kelsen_data.json %s", output_filename);
        
        if (verbose) {
            printf("Executing: %s\n", command);
        }
        
        /* Run the command */
        int result = system(command);
        
        if (result != 0) {
            fprintf(stderr, "Kelsen validation failed with exit code %d\n", result);
        } else if (verbose) {
            printf("Kelsen validation successful\n");
        }
    } else {
        printf("%s", kelsen_code);
    }
    
    /* Clean up */
    free(kelsen_code);
    free_schema(schema);
    if (context_filename != NULL) {
        legal_context_cleanup();
    }
    config_cleanup();
    
    return EXIT_SUCCESS;
}
