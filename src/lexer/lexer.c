#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Constant for max token length
#define MAX_TOKEN_LEN 256

// DFA States
typedef enum {
    STATE_START,
    STATE_NUMERIC,
    STATE_IDENTIFIER,
    STATE_ERROR
} State;

// All Token Types
typedef enum {
    TOKEN_KEYWORD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMERIC,
    TOKEN_OPERATOR,
    TOKEN_DELIMITER,
    TOKEN_INVALID
} TokenType;

char *token_types[] = {"Keyword", "Identifier", "Numeric Literal", "Operator", "Delimiter", "Invalid"};

// C Subset Keywords
char *keywords[] = {"int", "char", "if", "else", "while", "for", "do", "return"};
int num_keywords = 8;

// Variables used for tracking overall token statistics
int max_token_len_seen = 0;
int token_type_counts[6] = {0};
int total_tokens = 0, valid_tokens = 0;

/*
 * Takes a string/lexeme and returns true if that lexeme is a keyword and false if not.
 */
bool is_keyword(char *lexeme) {
    for (int i = 0; i < num_keywords; i++) {
        if (strcmp(keywords[i], lexeme) == 0) {
            return true;
        }
    }
    return false;
}

/*
 * Takes a character and returns true if it's any form of whitespace and false if not.
 * Whitespace includes spaces, tabs, newlines, and carriage returns.
 */
bool is_whitespace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*
 * Takes a character and returns true if it's a delimiter in our C subset and false if not.
 */
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

/*
 * Takes a character and returns true if it's an operator in our C subset and false if not.
 */
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

/*
 * Takes in a file pointer, count of lines, longest token, and seconds elapsed.
 * Writes to file and prints to stdout a list of summary statistics about tokens parsed.
 */
void print_summary(FILE *out, int line_count, int longest_token, double seconds_elapsed) {
    fprintf(out, "\n");
    printf("\n");

    fprintf(out, "================================ Summary Statistics ================================\n");
    printf("================================ Summary Statistics ================================\n");

    fprintf(out, "Total Tokens: %d, Total Valid Tokens: %d, Total Invalid Tokens: %d\n", total_tokens,
            valid_tokens, token_type_counts[5]);
    printf("Total Tokens: %d, Total Valid Tokens: %d, Total Invalid Tokens: %d\n", total_tokens, valid_tokens,
           token_type_counts[5]);

    fprintf(out, "Total Keywords: %d\n", token_type_counts[0]);
    printf("Total Keywords: %d\n", token_type_counts[0]);

    fprintf(out, "Total Identifiers: %d\n", token_type_counts[1]);
    printf("Total Identifiers: %d\n", token_type_counts[1]);

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

    fprintf(out, "Time Elapsed: %.6f seconds\n", seconds_elapsed);
    printf("Time Elapsed: %.6f seconds\n", seconds_elapsed);
}

/*
 * Takes in a file pointer, lexeme/string, token type, row, and column.
 * Increments token counts for statistical summary.
 * Writes to file and prints to screen the token information.
 */
void emit_token(FILE *out, char *lexeme, TokenType type, int row, int col) {
    total_tokens++;
    valid_tokens += type == TOKEN_INVALID ? 0 : 1;
    token_type_counts[type]++;

    fprintf(out, "Lexeme: %s, Token Type: %s, Row: %d, Column: %d\n", lexeme, token_types[type], row, col);
    printf("Lexeme: %s, Token Type: %s, Row: %d, Column: %d\n", lexeme, token_types[type], row, col);
}

/*
 * Takes in a file pointer, character pointer to where the token starts, character pointer to where the token
 * ends, row number, column number, and current state.
 * Trims token to MAX_TOKEN_LEN if it exceeds it. Then based on state will call emit_token with the correct
 * state information.
 */
void flush_token(FILE *out, char *token_start, char *current, int row, int col, State state) {
    // No actual token
    if (token_start == NULL) {
        return;
    }

    int len = current - token_start;

    // Invalid length
    if (len <= 0) {
        return;
    }

    // Updates max seen token length
    if (len > max_token_len_seen) {
        max_token_len_seen = len;
    }

    // Enforces MAX_TOKEN_LEN
    if (len > MAX_TOKEN_LEN) {
        len = MAX_TOKEN_LEN;
    }

    // Copies token to a separate character buffer
    char token[MAX_TOKEN_LEN + 1];
    memcpy(token, token_start, len);
    token[len] = '\0';

    switch (state) {
    case STATE_IDENTIFIER:
        // Since both identifiers and keywords are recognised by the STATE_IDENTIFIER state, we use a
        // boolean ternary operator to decide which one gets emitted.
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

/*
 * Takes in a character buffer for the whole input file. Is responsible for parsing the entire input file.
 * Simulates the DFA process through direct-coded switch and if-else blocks to perform state transitions.
*/
void parse(char *buffer) {
    // Starts the clock for counting seconds elapsed
    clock_t start = clock();
    
    // Opens a new file called tokens.txt, checks if successful, if not returns and prints error
    FILE *out = fopen("tokens.txt", "w");
    if (out == NULL) {
        fprintf(stderr, "Error: failed to open output file\n");
        return;
    }
    
    // Initialises the buffer, row/col indices, and the start state
    char *current = buffer;
    char *token_start = NULL;
    int row = 0, col = 0, token_col = 0;
    State state = STATE_START;

    // Main loop continues until null-terminator is encountered
    while (*current != '\0') {
        char c = *current;
        
        // DFA State Transitions
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
                // Ensures row and column information are accurately handled
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
            token_start = NULL;
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
            token_start = NULL;
            state = STATE_START;
            continue;

        case STATE_ERROR:
            if (is_whitespace(c)) {
                flush_token(out, token_start, current, row, token_col, state);
                token_start = NULL;
                state = STATE_START;
                continue;
            }
            break;
        }

        current++;
        col++;
    }
    
    // Final flush to ensure last token is processed correctly
    flush_token(out, token_start, current, row, token_col, state);
    token_start = NULL;
    
    // End timer and compute seconds elapsed
    clock_t end = clock();
    double seconds_elapsed = (double)(end - start) / CLOCKS_PER_SEC;
    
    print_summary(out, row + 1, max_token_len_seen, seconds_elapsed);
    
    // Ensures resource is closed before function exits, file pointer is not leaked
    fclose(out);
}

int main(int argc, char *argv[]) {
    // If no file is given as CLI argument, program will return EXIT_FAILURE
    if (argc < 2) {
        fprintf(stderr, "No file to parse! Please ensure the relative/absolute "
                        "file path is a CLI program argument.\n");
        return EXIT_FAILURE;
    }

    FILE *input_file = fopen(argv[1], "r");
    
    // Checks if the file is openable
    if (input_file == NULL) {
        fprintf(stderr, "Error: loading file, ensure file name is correct "
                        "relative/absolute path.\n");
        return EXIT_FAILURE;
    }
    
    // Gets file size information
    fseek(input_file, 0, SEEK_END);
    long file_size = ftell(input_file);
    rewind(input_file);
    
    // Allocates a character buffer for the file and checks if the allocation was successful
    char *buffer = malloc(file_size + 1);
    if (buffer == NULL) {
        fprintf(stderr, "Error: Buffer allocation failed\n");
        fclose(input_file);
        return EXIT_FAILURE;
    }
    
    // Writes the file character by character to the character buffer
    size_t bytes_read = fread(buffer, 1, file_size, input_file);
    buffer[bytes_read] = '\0';
    
    parse(buffer);
    
    // Closes the file pointer and frees the heap allocated memory for the buffer
    fclose(input_file);
    free(buffer);

    return EXIT_SUCCESS;
}
