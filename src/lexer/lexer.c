#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_LEN 256

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

int max_token_len_seen = 0;
int token_type_counts[6] = {0};
int total_tokens = 0, valid_tokens = 0;

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

void print_summary(FILE *out, int line_count, int longest_token) {
    fprintf(out, "============ Summary Statistics ============");
    printf("============ Summary Statistics ============");

    fprintf(out, "Total Tokens: %d, Total Valid Tokens: %d, Total Invalid Tokens: %d\n", total_tokens,
            valid_tokens, token_type_counts[5]);
    printf("Total Tokens: %d, Total Valid Tokens: %d, Total Invalid Tokens: %d\n", total_tokens, valid_tokens,
           token_type_counts[5]);

    fprintf(out, "Total Keywords: %d\n", token_type_counts[0]);
    printf("Total Keywords: %d\n", token_type_counts[0]);

    fprintf(out, "Total Indentifiers: %d\n", token_type_counts[1]);
    printf("Total Indentifiers: %d\n", token_type_counts[1]);

    fprintf(out, "Total Numeric Literals: %d\n", token_type_counts[2]);
    printf("Total Numeric Literals: %d\n", token_type_counts[2]);

    fprintf(out, "Total Operators: %d\n", token_type_counts[3]);
    printf("Total Operators: %d\n", token_type_counts[3]);

    fprintf(out, "Total Delimiters: %d\n", token_type_counts[4]);
    printf("Total Delimiters: %d\n", token_type_counts[4]);

    fprintf(out, "Total Lines Consumed: %d\n", line_count);
    printf("Total Lines Consumed: %d\n", line_count);

    fprintf(out, "Longest Token Seen was %d Characters\n", longest_token);
    printf("Longest Token Seen was %d Characters\n", longest_token);
}

void emit_token(FILE *out, char *lexeme, TokenType type, int row, int col) {
    total_tokens++;
    valid_tokens++;
    token_type_counts[type]++;

    fprintf(out, "Lexeme: %s, Token Type: %s, Row: %d, Column: %d\n", lexeme, token_types[type], row, col);
    printf("Lexeme: %s, Token Type: %s, Row: %d, Column: %d\n", lexeme, token_types[type], row, col);
}

void flush_token(FILE *out, char *token_start, char *current, int row, int col, State state) {
    int len = current - token_start;
    if (len <= 0) {
        return;
    }

    if (len > max_token_len_seen) {
        max_token_len_seen = len;
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

void parse(char *buffer) {
    FILE *out = fopen("tokens.txt", "w");
    if (out == NULL) {
        fprintf(stderr, "Error: failed to open output file\n");
        return;
    }

    char *current = buffer;
    char *token_start = NULL;
    int row = 0, col = 0, token_col = 0;
    State state = STATE_START;

    while (*current != '\0') {
        char c = *current;

        switch (state) {
        case STATE_START:
            if (isalpha(c) || c == '_') {
                token_start = current;
                token_col = col;
                state = STATE_IDENTIFIER;
            } else if (isdigit(c)) {
                token_start = current;
                token_col = col;
                state = STATE_NUMERIC;
            } else if (is_operator(c)) {
                char op[] = {c, '\0'};
                emit_token(out, op, TOKEN_OPERATOR, row, col);
            } else if (is_delimiter(c)) {
                char delim[] = {c, '\0'};
                emit_token(out, delim, TOKEN_DELIMITER, row, col);
            } else if (is_whitespace(c)) {
                if (c == '\n') {
                    row++;
                    col = -1;
                }
            } else {
                token_start = current;
                token_col = col;
                state = STATE_ERROR;
            }
            break;

        case STATE_IDENTIFIER:
            if (isalnum(c) || c == '_') {
                break;
            }

            flush_token(out, token_start, current, row, token_col, state);
            state = STATE_START;
            continue;

        case STATE_NUMERIC:
            if (isdigit(c)) {
                break;
            }
            if (isalpha(c) || c == '_') {
                state = STATE_ERROR;
                break;
            }

            flush_token(out, token_start, current, row, token_col, state);
            state = STATE_START;
            continue;

        case STATE_ERROR:
            if (is_whitespace(c)) {
                flush_token(out, token_start, current, row, token_col, state);
                state = STATE_START;
                continue;
            }
            break;
        }

        current++;
        col++;
    }

    flush_token(out, token_start, current, row, token_col, state);
    print_summary(out, row, max_token_len_seen);
    
    fclose(out);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "No file to parse! Please ensure the relative/absolute "
                        "file path is a CLI program argument.\n");
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "r");

    if (input_file == NULL) {
        fprintf(stderr, "Error: loading file, ensure file name is correct "
                        "relative/absolute path.\n");
        return EXIT_FAILURE;
    }

    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    rewind(input_file);

    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Buffer allocation failed\n");
        fclose(input_file);
        return EXIT_FAILURE;
    }

    size_t bytes_read = fread(buffer, 1, file_size, input_file);
    buffer[bytes_read] = '\0';

    parse(buffer);

    fclose(input_file);
    free(buffer);

    return EXIT_SUCCESS;
}
