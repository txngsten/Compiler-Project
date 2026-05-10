#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_LEN 512

typedef enum {
    STATE_START,
    STATE_NUMERIC,
    STATE_IDENTIFIER,
    STATE_ERROR
} State;

typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMERIC,
    TOKEN_OPERATOR,
    TOKEN_DELIMITER,
    TOKEN_INVALID
} TokenType;

char *token_types[] = {"Keyword", "Identifier", "Numeric Literal", "Operator", "Delimiter", "Invalid"};

char *keywords[] = {"int", "char", "if", "else", "while", "for", "do", "return"};
int num_keywords = 8;

bool is_keyword(char *lexeme) {
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(keywords[i], lexeme) == 0) {
            return true;
        }
    }
    return false;
}

bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
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
    case '<':
        return true;
    case '>':
        return true;
    case '*':
        return true;
    case '/':
        return true;
    }

    return false;
}

void emit_token(FILE *out, char *lexeme, TokenType type, int row, int col) {
    fprintf(out, "Lexeme: %s, Token Type: %s, Row: %d, Column: %d\n", lexeme, token_types[type], row, col);
    printf("Lexeme: %s, Token Type: %s, Row: %d, Column: %d\n", lexeme, token_types[type], row, col);
}

void flush_token(FILE *out, char *token_start, char *current, int row, int col, State state) {
    int len = current - token_start;
    if (len <= 0) {
        return;
    }

    if (len > MAX_TOKEN_LEN) {
        len = MAX_TOKEN_LEN;
    }

    char token[MAX_TOKEN_LEN + 1];
    memcpy(token, token_start, len);
    token[len] = '\0';

    switch (state) {
        case STATE_IDENTIFIER:
            emit_token(out, token, is_keyword(token) ? TOKEN_KEYWORD : TOKEN_IDENTIFIER, row, col);
            break;
        case STATE_NUMERIC:
            emit_token(out, token, TOKEN_NUMERIC, row, col);
            break;
        case STATE_ERROR:
            emit_token(out, token, TOKEN_INVALID, row, col);
            break;
        default:
            break;
    }
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
