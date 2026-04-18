CC = clang
CFLAGS = -Wall -Wextra -std=c11

TARGET = lexer
SRC = lexer.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
