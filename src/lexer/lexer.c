#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Token {
    char *lexeme;
    char *token_type;
    int row;
    int col_start;
    int col_end;
};

char *keywords[] = {"int",   "char", "if", "else",
                    "while", "for",  "do", "return"};
int num_keywords = 8;

bool is_keyword(char *lexeme) {
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(keywords[i], lexeme) == 0) {
            return true;
        }
    }
    return false;
}

bool is_delimiter(char c) {
    switch (c) {
    case ';':
        return true;
    case ',':
        return true;
    case '(':
        return true;
    case ')':
        return true;
    case '{':
        return true;
    case '}':
        return true;
    case '[':
        return true;
    case ']':
        return true;
    }

    return false;
}

bool is_operator(char c) {
    switch (c) {
    case '+':
        return true;
    case '-':
        return true;
    case '=':
        return true;
    }

    return false;
}

void tokenisation(char *buffer, size_t buffer_size) {
    char *start = buffer;

}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No file to parse! Please ensure the relative/absolute "
                        "file path is a CLI program argument.\n");
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        fprintf(stderr, "Error loading file, ensure file name is correct "
                        "relative/absolute path.\n");
        return EXIT_FAILURE;
    }

    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    rewind(input_file);

    char *buffer = malloc(file_size + 1);

    size_t bytes_read = fread(buffer, 1, file_size, input_file);
    buffer[bytes_read] = '\0';

    fclose(input_file);
    free(buffer);

    return EXIT_SUCCESS;
}
