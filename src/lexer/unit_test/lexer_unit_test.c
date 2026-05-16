#include "unity.h"

/*
 * Include lexer.c directly to access internal functions without a header.
 * Rename its main() to avoid conflicting with Unity's test runner main().
 */
#define main lexer_main
#include "../lexer.c"
#undef main

/* Temp file used by emit_token/flush_token tests */
#define TEST_OUTPUT_FILE "test_output.tmp"

/* ========================= Test Helpers ========================= */

/*
 * Resets all global state between tests so each test starts clean.
 */
void reset_globals(void) {
    total_tokens = 0;
    valid_tokens = 0;
    max_token_len_seen = 0;
    for (int i = 0; i < 6; i++) {
        token_type_counts[i] = 0;
    }
}

/*
 * Reads entire file into a malloc'd string. Caller must free().
 * Returns NULL if the file cannot be opened.
 */
char *read_file_contents(const char *path) {
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);

    char *buf = malloc(size + 1);
    if (buf == NULL) {
        fclose(f);
        return NULL;
    }

    size_t bytes = fread(buf, 1, size, f);
    buf[bytes] = '\0';
    fclose(f);
    return buf;
}

/* ========================= setUp / tearDown ========================= */

void setUp(void) {
    reset_globals();
}

void tearDown(void) {
    remove(TEST_OUTPUT_FILE);
    remove("tokens.txt");
}

/* ========================= is_keyword Tests ========================= */

void test_is_keyword_all_keywords(void) {
    TEST_ASSERT_TRUE(is_keyword("int"));
    TEST_ASSERT_TRUE(is_keyword("char"));
    TEST_ASSERT_TRUE(is_keyword("if"));
    TEST_ASSERT_TRUE(is_keyword("else"));
    TEST_ASSERT_TRUE(is_keyword("while"));
    TEST_ASSERT_TRUE(is_keyword("for"));
    TEST_ASSERT_TRUE(is_keyword("do"));
    TEST_ASSERT_TRUE(is_keyword("return"));
}

void test_is_keyword_rejects_non_keywords(void) {
    TEST_ASSERT_FALSE(is_keyword("main"));
    TEST_ASSERT_FALSE(is_keyword("printf"));
    TEST_ASSERT_FALSE(is_keyword("x"));
}

void test_is_keyword_case_sensitive(void) {
    TEST_ASSERT_FALSE(is_keyword("INT"));
    TEST_ASSERT_FALSE(is_keyword("If"));
    TEST_ASSERT_FALSE(is_keyword("RETURN"));
}

void test_is_keyword_rejects_empty(void) {
    TEST_ASSERT_FALSE(is_keyword(""));
}

/* ========================= is_whitespace Tests ========================= */

void test_is_whitespace_accepts_all(void) {
    TEST_ASSERT_TRUE(is_whitespace(' '));
    TEST_ASSERT_TRUE(is_whitespace('\t'));
    TEST_ASSERT_TRUE(is_whitespace('\n'));
    TEST_ASSERT_TRUE(is_whitespace('\r'));
}

void test_is_whitespace_rejects_non_whitespace(void) {
    TEST_ASSERT_FALSE(is_whitespace('a'));
    TEST_ASSERT_FALSE(is_whitespace('0'));
    TEST_ASSERT_FALSE(is_whitespace('+'));
    TEST_ASSERT_FALSE(is_whitespace('\0'));
}

/* ========================= is_delimiter Tests ========================= */

void test_is_delimiter_accepts_all(void) {
    TEST_ASSERT_TRUE(is_delimiter(';'));
    TEST_ASSERT_TRUE(is_delimiter(','));
    TEST_ASSERT_TRUE(is_delimiter('('));
    TEST_ASSERT_TRUE(is_delimiter(')'));
    TEST_ASSERT_TRUE(is_delimiter('{'));
    TEST_ASSERT_TRUE(is_delimiter('}'));
    TEST_ASSERT_TRUE(is_delimiter('['));
    TEST_ASSERT_TRUE(is_delimiter(']'));
}

void test_is_delimiter_rejects_non_delimiters(void) {
    TEST_ASSERT_FALSE(is_delimiter('+'));
    TEST_ASSERT_FALSE(is_delimiter('a'));
    TEST_ASSERT_FALSE(is_delimiter(' '));
    TEST_ASSERT_FALSE(is_delimiter('.'));
}

/* ========================= is_operator Tests ========================= */

void test_is_operator_accepts_all(void) {
    TEST_ASSERT_TRUE(is_operator('+'));
    TEST_ASSERT_TRUE(is_operator('-'));
    TEST_ASSERT_TRUE(is_operator('='));
    TEST_ASSERT_TRUE(is_operator('<'));
    TEST_ASSERT_TRUE(is_operator('>'));
    TEST_ASSERT_TRUE(is_operator('*'));
    TEST_ASSERT_TRUE(is_operator('/'));
}

void test_is_operator_rejects_non_operators(void) {
    TEST_ASSERT_FALSE(is_operator(';'));
    TEST_ASSERT_FALSE(is_operator('a'));
    TEST_ASSERT_FALSE(is_operator(' '));
    TEST_ASSERT_FALSE(is_operator('^'));
}

/* ========================= emit_token Tests ========================= */

void test_emit_token_increments_total(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    emit_token(out, "x", TOKEN_IDENTIFIER, 0, 0);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
}

void test_emit_token_valid_increments_valid_count(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    emit_token(out, "int", TOKEN_KEYWORD, 0, 0);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, valid_tokens);
}

void test_emit_token_invalid_does_not_increment_valid(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    emit_token(out, "^", TOKEN_INVALID, 0, 0);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
    TEST_ASSERT_EQUAL_INT(0, valid_tokens);
}

void test_emit_token_increments_correct_type_count(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    emit_token(out, "42", TOKEN_NUMERIC, 0, 0);
    emit_token(out, "+", TOKEN_OPERATOR, 0, 3);
    emit_token(out, "+", TOKEN_OPERATOR, 0, 5);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_NUMERIC]);
    TEST_ASSERT_EQUAL_INT(2, token_type_counts[TOKEN_OPERATOR]);
    TEST_ASSERT_EQUAL_INT(0, token_type_counts[TOKEN_KEYWORD]);
}

void test_emit_token_writes_correct_format(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    emit_token(out, "foo", TOKEN_IDENTIFIER, 3, 7);
    fclose(out);

    char *contents = read_file_contents(TEST_OUTPUT_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: foo"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Token Type: Identifier"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Row: 3"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Column: 7"));
    free(contents);
}

/* ========================= flush_token Tests ========================= */

void test_flush_token_null_start_does_nothing(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char dummy[] = "abc";
    flush_token(out, NULL, dummy, 0, 0, STATE_IDENTIFIER);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(0, total_tokens);
}

void test_flush_token_zero_length_does_nothing(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char input[] = "abc";
    /* start == current means length 0 */
    flush_token(out, input, input, 0, 0, STATE_IDENTIFIER);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(0, total_tokens);
}

void test_flush_token_identifier(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char input[] = "myVar ";
    flush_token(out, input, input + 5, 0, 0, STATE_IDENTIFIER);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_IDENTIFIER]);

    char *contents = read_file_contents(TEST_OUTPUT_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: myVar"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Token Type: Identifier"));
    free(contents);
}

void test_flush_token_keyword_via_identifier_state(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char input[] = "while ";
    flush_token(out, input, input + 5, 0, 0, STATE_IDENTIFIER);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_KEYWORD]);
    TEST_ASSERT_EQUAL_INT(0, token_type_counts[TOKEN_IDENTIFIER]);

    char *contents = read_file_contents(TEST_OUTPUT_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Token Type: Keyword"));
    free(contents);
}

void test_flush_token_numeric(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char input[] = "1234 ";
    flush_token(out, input, input + 4, 0, 0, STATE_NUMERIC);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_NUMERIC]);

    char *contents = read_file_contents(TEST_OUTPUT_FILE);
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: 1234"));
    free(contents);
}

void test_flush_token_error(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char input[] = "^ ";
    flush_token(out, input, input + 1, 0, 0, STATE_ERROR);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_INVALID]);
    TEST_ASSERT_EQUAL_INT(0, valid_tokens);
}

void test_flush_token_updates_max_len(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char input[] = "abcdef ";
    flush_token(out, input, input + 6, 0, 0, STATE_IDENTIFIER);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(6, max_token_len_seen);
}

void test_flush_token_state_start_does_nothing(void) {
    FILE *out = fopen(TEST_OUTPUT_FILE, "w");
    TEST_ASSERT_NOT_NULL(out);

    char input[] = "abc";
    flush_token(out, input, input + 3, 0, 0, STATE_START);
    fclose(out);

    TEST_ASSERT_EQUAL_INT(0, total_tokens);
}

/* ========================= parse() End-to-End Tests ========================= */

void test_parse_single_keyword(void) {
    char input[] = "int";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_KEYWORD]);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: int"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Token Type: Keyword"));
    free(contents);
}

void test_parse_single_identifier(void) {
    char input[] = "myVar";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_IDENTIFIER]);
}

void test_parse_identifier_with_underscore_prefix(void) {
    char input[] = "_count";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_IDENTIFIER]);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: _count"));
    free(contents);
}

void test_parse_numeric_literal(void) {
    char input[] = "42";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_NUMERIC]);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: 42"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Token Type: Numeric Literal"));
    free(contents);
}

void test_parse_all_operators(void) {
    char input[] = "+ - = < > * /";
    parse(input);

    TEST_ASSERT_EQUAL_INT(7, token_type_counts[TOKEN_OPERATOR]);
    TEST_ASSERT_EQUAL_INT(7, total_tokens);
}

void test_parse_all_delimiters(void) {
    char input[] = "; , ( ) { } [ ]";
    parse(input);

    TEST_ASSERT_EQUAL_INT(8, token_type_counts[TOKEN_DELIMITER]);
    TEST_ASSERT_EQUAL_INT(8, total_tokens);
}

void test_parse_empty_input(void) {
    char input[] = "";
    parse(input);

    TEST_ASSERT_EQUAL_INT(0, total_tokens);
}

void test_parse_whitespace_only(void) {
    char input[] = "   \t\n\r  ";
    parse(input);

    TEST_ASSERT_EQUAL_INT(0, total_tokens);
}

void test_parse_standalone_invalid_char(void) {
    char input[] = "^ ";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
    TEST_ASSERT_EQUAL_INT(0, valid_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_INVALID]);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: ^"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Token Type: Invalid"));
    free(contents);
}

void test_parse_numeric_alpha_error_recovery(void) {
    /* 123abc should be emitted as a single invalid token */
    char input[] = "123abc ";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_INVALID]);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: 123abc"));
    free(contents);
}

void test_parse_mixed_valid_tokens(void) {
    char input[] = "int x = 10;";
    parse(input);

    TEST_ASSERT_EQUAL_INT(5, total_tokens);
    TEST_ASSERT_EQUAL_INT(5, valid_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_KEYWORD]);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_IDENTIFIER]);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_OPERATOR]);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_NUMERIC]);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_DELIMITER]);
}

void test_parse_row_tracking_across_newlines(void) {
    char input[] = "int\nx";
    parse(input);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    /* "int" should be on row 0, "x" should be on row 1 */
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: int, Token Type: Keyword, Row: 0"));
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: x, Token Type: Identifier, Row: 1"));
    free(contents);
}

void test_parse_column_tracking(void) {
    char input[] = "  int";
    parse(input);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    /* Two leading spaces means "int" starts at column 2 */
    TEST_ASSERT_NOT_NULL(strstr(contents, "Column: 2"));
    free(contents);
}

void test_parse_multiple_invalid_chars_grouped(void) {
    /* "^^" with no whitespace in between should group into one invalid token */
    char input[] = "^^ ";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_INVALID]);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Lexeme: ^^"));
    free(contents);
}

void test_parse_invalid_then_valid_recovery(void) {
    /* Panic mode: invalid char skipped, then valid token recognised */
    char input[] = "^ int";
    parse(input);

    TEST_ASSERT_EQUAL_INT(2, total_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_INVALID]);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_KEYWORD]);
}

void test_parse_adjacent_delimiters_and_operators(void) {
    char input[] = "(){+}";
    parse(input);

    TEST_ASSERT_EQUAL_INT(5, total_tokens);
    TEST_ASSERT_EQUAL_INT(4, token_type_counts[TOKEN_DELIMITER]);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_OPERATOR]);
}

void test_parse_summary_reports_line_count(void) {
    char input[] = "a\nb\nc";
    parse(input);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    /* Three lines: rows 0, 1, 2 → summary should say 3 lines consumed */
    TEST_ASSERT_NOT_NULL(strstr(contents, "Total Lines Consumed: 3"));
    free(contents);
}

void test_parse_summary_reports_longest_token(void) {
    char input[] = "a longIdentifier b";
    parse(input);

    char *contents = read_file_contents("tokens.txt");
    TEST_ASSERT_NOT_NULL(contents);
    TEST_ASSERT_NOT_NULL(strstr(contents, "Longest Token Seen was 14 Characters"));
    free(contents);
}

void test_parse_keyword_not_substring_match(void) {
    /* "interface" starts with "int" but should be an identifier, not a keyword */
    char input[] = "interface";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_IDENTIFIER]);
    TEST_ASSERT_EQUAL_INT(0, token_type_counts[TOKEN_KEYWORD]);
}

void test_parse_token_at_end_of_input_no_trailing_space(void) {
    /* Final flush should handle a token right at the null terminator */
    char input[] = "return";
    parse(input);

    TEST_ASSERT_EQUAL_INT(1, total_tokens);
    TEST_ASSERT_EQUAL_INT(1, token_type_counts[TOKEN_KEYWORD]);
}

/* ========================= Test Runner ========================= */

int main(void) {
    UNITY_BEGIN();

    /* is_keyword */
    RUN_TEST(test_is_keyword_all_keywords);
    RUN_TEST(test_is_keyword_rejects_non_keywords);
    RUN_TEST(test_is_keyword_case_sensitive);
    RUN_TEST(test_is_keyword_rejects_empty);

    /* is_whitespace */
    RUN_TEST(test_is_whitespace_accepts_all);
    RUN_TEST(test_is_whitespace_rejects_non_whitespace);

    /* is_delimiter */
    RUN_TEST(test_is_delimiter_accepts_all);
    RUN_TEST(test_is_delimiter_rejects_non_delimiters);

    /* is_operator */
    RUN_TEST(test_is_operator_accepts_all);
    RUN_TEST(test_is_operator_rejects_non_operators);

    /* emit_token */
    RUN_TEST(test_emit_token_increments_total);
    RUN_TEST(test_emit_token_valid_increments_valid_count);
    RUN_TEST(test_emit_token_invalid_does_not_increment_valid);
    RUN_TEST(test_emit_token_increments_correct_type_count);
    RUN_TEST(test_emit_token_writes_correct_format);

    /* flush_token */
    RUN_TEST(test_flush_token_null_start_does_nothing);
    RUN_TEST(test_flush_token_zero_length_does_nothing);
    RUN_TEST(test_flush_token_identifier);
    RUN_TEST(test_flush_token_keyword_via_identifier_state);
    RUN_TEST(test_flush_token_numeric);
    RUN_TEST(test_flush_token_error);
    RUN_TEST(test_flush_token_updates_max_len);
    RUN_TEST(test_flush_token_state_start_does_nothing);

    /* parse end-to-end */
    RUN_TEST(test_parse_single_keyword);
    RUN_TEST(test_parse_single_identifier);
    RUN_TEST(test_parse_identifier_with_underscore_prefix);
    RUN_TEST(test_parse_numeric_literal);
    RUN_TEST(test_parse_all_operators);
    RUN_TEST(test_parse_all_delimiters);
    RUN_TEST(test_parse_empty_input);
    RUN_TEST(test_parse_whitespace_only);
    RUN_TEST(test_parse_standalone_invalid_char);
    RUN_TEST(test_parse_numeric_alpha_error_recovery);
    RUN_TEST(test_parse_mixed_valid_tokens);
    RUN_TEST(test_parse_row_tracking_across_newlines);
    RUN_TEST(test_parse_column_tracking);
    RUN_TEST(test_parse_multiple_invalid_chars_grouped);
    RUN_TEST(test_parse_invalid_then_valid_recovery);
    RUN_TEST(test_parse_adjacent_delimiters_and_operators);
    RUN_TEST(test_parse_summary_reports_line_count);
    RUN_TEST(test_parse_summary_reports_longest_token);
    RUN_TEST(test_parse_keyword_not_substring_match);
    RUN_TEST(test_parse_token_at_end_of_input_no_trailing_space);

    return UNITY_END();
}
