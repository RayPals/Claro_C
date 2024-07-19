#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <setjmp.h>

#define MAX_CODE_LINES 1000
#define MAX_LINE_LENGTH 256
#define MAX_VARIABLES 100
#define MAX_FUNCTIONS 100

typedef enum {
    PRINT, VARIABLE, IF, ELSE, WHILE, END, INPUT, FUNC, CALL, LIST, DICT, STRING, COMMENT, TRY, EXCEPT, FINALLY, BREAK, CONTINUE, FILE_OP, FOR, IMPORT
} StmtType;

typedef struct {
    char name[MAX_LINE_LENGTH];
    char value[MAX_LINE_LENGTH];
} Variable;

typedef struct {
    char name[MAX_LINE_LENGTH];
    char code[MAX_CODE_LINES][MAX_LINE_LENGTH];
    int param_count;
    char params[MAX_VARIABLES][MAX_LINE_LENGTH];
} Function;

jmp_buf error_buf;

Variable variables[MAX_VARIABLES];
int variable_count = 0;

Function functions[MAX_FUNCTIONS];
int function_count = 0;

bool break_loop = false;
bool continue_loop = false;

void error(const char *message, int line_number) {
    fprintf(stderr, "Error on line %d: %s\n", line_number, message);
    longjmp(error_buf, 1);
}

char *trim_whitespace(char *str) {
    char *end;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void strip_comments(char *line) {
    char *comment_start = strchr(line, '#');
    if (comment_start) {
        *comment_start = '\0';
    }
}

int parse_code(const char *code, char parsed_code[MAX_CODE_LINES][MAX_LINE_LENGTH]) {
    int line_count = 0;
    char *line = strtok(strdup(code), "\n");
    while (line != NULL) {
        line = trim_whitespace(line);
        strip_comments(line);
        if (strlen(line) > 0) {
            strncpy(parsed_code[line_count++], line, MAX_LINE_LENGTH);
        }
        line = strtok(NULL, "\n");
    }
    return line_count;
}

char* get_variable(const char *name) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            return variables[i].value;
        }
    }
    return NULL;
}

void set_variable(const char *name, const char *value) {
    for (int i = 0; i < variable_count; i++) {
        if (strcmp(variables[i].name, name) == 0) {
            strncpy(variables[i].value, value, MAX_LINE_LENGTH);
            return;
        }
    }
    strncpy(variables[variable_count].name, name, MAX_LINE_LENGTH);
    strncpy(variables[variable_count].value, value, MAX_LINE_LENGTH);
    variable_count++;
}

void execute_line(char *line, int line_number) {
    line = trim_whitespace(line);
    if (strlen(line) == 0 || line[0] == '#') {
        // Skip empty lines or comments
        return;
    }
    char *command = strtok(line, " ");
    if (strcmp(command, "COMMENT") == 0) {
        // Skip comments
        return;
    } else if (strcmp(command, "PRINT") == 0) {
        char *arg = strtok(NULL, "\n");
        if (arg[0] == '"') {
            // Handle string literal
            if (arg[strlen(arg) - 1] == '"') {
                arg[strlen(arg) - 1] = '\0';  // Remove ending quote if present
            }
            printf("%s\n", arg + 1);  // Print without starting quote
        } else {
            // Handle variable
            char *value = get_variable(arg);
            if (value != NULL) {
                printf("%s\n", value);
            } else {
                error("Undefined variable", line_number);
            }
        }
    } else if (strcmp(command, "VARIABLE") == 0) {
        char *name = strtok(NULL, " =");
        strtok(NULL, " ");  // Skip the equal sign
        char *value = strtok(NULL, "\n");
        if (value) {
            strip_comments(value);  // Strip comments before processing the value
            value = trim_whitespace(value);  // Trim whitespace again after stripping comments
            if (value[0] == '"') {
                value++;
                if (value[strlen(value) - 1] == '"') {
                    value[strlen(value) - 1] = '\0';  // Remove ending quote if present
                }
            }
            set_variable(name, value);
        } else {
            error("Invalid variable assignment", line_number);
        }
    } else {
        // Handle other commands or error
        error("Unknown command", line_number);
    }
}

void execute_code(char code[MAX_CODE_LINES][MAX_LINE_LENGTH], int line_count) {
    for (int i = 0; i < line_count; i++) {
        execute_line(code[i], i + 1);
    }
}

void execute_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return;
    }

    char code[MAX_CODE_LINES][MAX_LINE_LENGTH];
    int line_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file) && line_count < MAX_CODE_LINES) {
        strncpy(code[line_count++], line, MAX_LINE_LENGTH);
    }
    fclose(file);
    execute_code(code, line_count);
}

void interactive_mode() {
    printf("Entering interactive mode (type 'exit' to quit)\n");
    char line[MAX_LINE_LENGTH];
    while (true) {
        printf("> ");
        if (fgets(line, sizeof(line), stdin) == NULL) break;
        trim_whitespace(line);
        strip_comments(line);
        if (strcmp(line, "exit") == 0) break;
        execute_line(line, 0);
    }
}

void print_help() {
    printf("Usage: claro [options]\n\n");
    printf("Options:\n");
    printf("  -e <file>      Execute the code from the specified file\n");
    printf("  -i             Enter interactive mode\n");
    printf("  -h, --help     Show this help message\n");
    printf("  --version      Show version information\n");
}

void print_version() {
    printf("Claro Interpreter Version 1.0\n");
}

int main(int argc, char *argv[]) {
    if (setjmp(error_buf) == 0) {
        if (argc == 1) {
            print_help();
            return 0;
        }
        if (argc < 2) {
            fprintf(stderr, "Error: Missing command-line arguments\n");
            print_help();
            return 1;
        }
        if (strcmp(argv[1], "-e") == 0) {
            if (argc != 3) {
                print_help();
                return 1;
            }
            execute_file(argv[2]);
        } else if (strcmp(argv[1], "-i") == 0) {
            interactive_mode();
        } else if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            print_help();
        } else if (strcmp(argv[1], "--version") == 0) {
            print_version();
        } else {
            fprintf(stderr, "Error: Invalid option\n");
            print_help();
            return 1;
        }
    }
    return 0;
}
