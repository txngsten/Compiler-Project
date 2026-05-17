CC = clang
CFLAGS = -Wall -Wextra -std=c11
SRC = lexer.c

.PHONY: all clean test

all: lexer

# For alpine container size optimisation
UNAME := $(shell uname)
ifeq ($(UNAME), Linux)
    LDFLAGS = -s -Wl,-z,max-page-size=4096
endif

lexer: $(SRC)
	$(CC) $(CFLAGS) -Os -flto $(LDFLAGS) -o lexer $(SRC)

clean:
	rm -f lexer unit_test/tests

TEST_SRC = unit_test/lexer_unit_test.c unit_test/unity.c

test: $(TEST_SRC) $(SRC)
	$(CC) $(CFLAGS) -I . -I unit_test -o unit_test/tests $(TEST_SRC)
	cd unit_test
