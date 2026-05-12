CC = clang
CFLAGS = -Wall -Wextra -std=c11
SRC = lexer.c

.PHONY: all clean

all: lexer

lexer: $(SRC)
	$(CC) $(CFLAGS) -O3 -flto -o lexer $(SRC)

clean:
	rm -f lexer
