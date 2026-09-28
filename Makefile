CC = gcc

CFLAGS = -Wall -Wextra -std=c11 -I./src/common

COMMON_SRC = src/common/protocol.c

COMMON_OBJ = $(COMMON_SRC:.c=.o)

all: $(COMMON_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(COMMON_OBJ)

.PHONY: all clean