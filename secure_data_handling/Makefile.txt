CC = gcc
CFLAGS = -std=gnu89 -Wall -Wextra -Werror -pedantic -g
SRC = main.c store.c session.c
OBJ = $(SRC:.c=.o)
BIN = secure_store

all: $(BIN)

$(BIN): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(BIN)

clean:
	rm -f $(OBJ) $(BIN)

re: clean all
