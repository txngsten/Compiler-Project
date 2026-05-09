#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TOKEN_LEN 512

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

void tokenise(FILE *out, char *token, int row, int col) {
    if (is_keyword(token)) {
        fprintf(out, "Lexeme: %s, Token Type: Keyword, Row Number: %d, Column: %d\n", token, row, col);
    } else if (token[0] == '_' || isalpha(token[0])) {
        fprintf(out, "Lexeme: %s, Token Type: Identifier, Row Number: %d, Column: %d\n", token, row, col);
    } else {
        fprintf(out, "Lexeme: %s, Token Type: Numeric Literal, Row Number: %d, Column: %d\n", token, row, col);
    }
}

void parse(char *buffer) {
    FILE *out = fopen("tokens.txt", "w");
    if (out == NULL) {
        fprintf(stderr, "Error: could not open output file\n");
        return;
    }

    char *start = buffer;
    char *token_start = buffer;

    int row = 0, col = 0;

    while (*start != '\0') {
        // Move index forward until non-space character or null-terminator
        while ((*token_start == ' ' || *token_start == '\n') && *token_start != '\0') {
            if (*token_start == '\n') {
                row++;
                col = 0;
                start++;
                token_start++;
                continue;
            }
            start++;
            token_start++;
            col++;
        }

        // Start processing token
        char *token_end = token_start;
        int token_col = col;

        while (*token_end != '\0' && *token_end != ' ' && *token_end != '\n') {
            // Invalid character, current token becomes invalid move onto the next one
            if ((!is_operator(*token_end) && !isalpha(*token_end) && !isdigit(*token_end) &&
                 !is_delimiter(*token_end)) &&
                *token_end != '_') {
                fprintf(out, "Invalid token: %.*s, Located at Row: %d, Column Start: %d\n",
                        (int)(token_end - token_start), token_start, row, token_col);

                col++;
                token_end++;
                token_start = token_end;
                break;
            }

            // Current character is an operator, parse the preceeding string and the operator after
            if (is_operator(*token_end)) {
                char op = *token_end;

                int len = token_end - token_start;

                if (len > 0) {
                    char token[MAX_TOKEN_LEN + 1];

                    memcpy(token, token_start, len);
                    token[len] = '\0';

                    tokenise(out, token, row, token_col);
                }

                fprintf(out, "Lexeme: %c, Token Type: Operator, Row Number: %d, Column: %d\n", op, row, col);

                col++;
                token_end++;
                token_start = token_end;
                break;
            }

            if (is_delimiter(*token_end)) {
                char delim = *token_end;

                int len = token_end - token_start;

                if (len > 0) {
                    char token[MAX_TOKEN_LEN + 1];

                    memcpy(token, token_start, len);
                    token[len] = '\0';

                    tokenise(out, token, row, token_col);
                }

                fprintf(out, "Lexeme: %c, Token Type: Delimiter, Row Number: %d, Column: %d\n", delim, row, col);

                col++;
                token_end++;
                token_start = token_end;
                break;
            }

            col++;
            token_end++;
        }

        int len = token_end - token_start;

        if (len > 0) {
            char token[MAX_TOKEN_LEN + 1];

            memcpy(token, token_start, len);
            token[len] = '\0';

            tokenise(out, token, row, token_col);
        }
        start = token_end;
        token_start = token_end;
    }

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

    parse(buffer);

    fclose(input_file);
    free(buffer);

    return EXIT_SUCCESS;
}
