CC = clang
CFLAGS = -Wall -Wextra -std=c11
SRC = lexer.c

.PHONY: all clean test

all: lexer

lexer: $(SRC)
	$(CC) $(CFLAGS) -O3 -flto -o lexer $(SRC)

clean:
	rm -f lexer unit_test/tests

TEST_SRC = unit_test/lexer_unit_test.c unit_test/unity.c

test: $(TEST_SRC) $(SRC)
	$(CC) $(CFLAGS) -I . -I unit_test -o unit_test/tests $(TEST_SRC)
	cd unit_test
