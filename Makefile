CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -I./src/common -I./src/storage

COMMON_SRC = src/common/protocol.c
STORAGE_SRC = src/storage/sha256.c

COMMON_OBJ = $(COMMON_SRC:.c=.o)
STORAGE_OBJ = $(STORAGE_SRC:.c=.o)

all: $(COMMON_OBJ) $(STORAGE_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(COMMON_OBJ) $(STORAGE_OBJ)

.PHONY: all clean
