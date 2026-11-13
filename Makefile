CC = gcc
CFLAGS = -Wall -g -Iinclude

SRC = src/main.c src/shell.c src/execute.c
OBJ = $(SRC:.c=.o)
EXEC = bin/myshell

all: $(EXEC)

$(EXEC): $(OBJ)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $@ $^ -lreadline

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f src/*.o bin/myshell
