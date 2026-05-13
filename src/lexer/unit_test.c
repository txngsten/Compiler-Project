// unit_test.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define MAX_LINE 1024

int main(int argc, char *argv[])
{
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <actual_output> <expected_output>\n", argv[0]);
        return 1;
    }

    FILE *actual = fopen(argv[1], "r");
    FILE *expected = fopen(argv[2], "r");

    if (!actual) {
        fprintf(stderr, "Could not open actual file: %s\n", argv[1]);
        return 1;
    }
    if (!expected) {
        fprintf(stderr, "Could not open expected file: %s\n", argv[2]);
        fclose(actual);
        return 1;
    }

    char act_line[MAX_LINE];
    char exp_line[MAX_LINE];
    int line_num = 0;
    int passed = 0;
    int failed = 0;

    while (true) {
        char *a = fgets(act_line, MAX_LINE, actual);
        char *e = fgets(exp_line, MAX_LINE, expected);

        /* Both files ended — we're done */
        if (!a && !e)
            break;

        line_num++;

        if (!a) {
            fprintf(stderr, "[FAIL] Line %d: actual file ended early. "
                            "Expected: %s", line_num, exp_line);
            failed++;
            continue;
        }
        if (!e) {
            fprintf(stderr, "[FAIL] Line %d: expected file ended early. "
                            "Got extra: %s", line_num, act_line);
            failed++;
            continue;
        }

        /* Strip trailing newlines for cleaner comparison */
        act_line[strcspn(act_line, "\n")] = '\0';
        exp_line[strcspn(exp_line, "\n")] = '\0';

        if (strcmp(act_line, exp_line) == 0) {
            printf("[PASS] Line %d: %s\n", line_num, act_line);
            passed++;
        } else {
            fprintf(stderr, "[FAIL] Line %d:\n"
                            "  expected: %s\n"
                            "  actual:   %s\n",
                            line_num, exp_line, act_line);
            failed++;
        }
    }

    printf("\n--- Results: %d passed, %d failed, %d total ---\n",
           passed, failed, passed + failed);

    fclose(actual);
    fclose(expected);

    return failed > 0 ? 1 : 0;
}
