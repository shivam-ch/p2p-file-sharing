CC = gcc

CFLAGS = -Wall -Wextra -std=c11 \
         -I./src/common \
         -I./src/storage \
         -I./src/transfer

LDFLAGS = -pthread -lcrypto

COMMON_SRC = src/common/protocol.c

STORAGE_SRC = \
	src/storage/piece.c \
	src/storage/sha256.c \
	src/storage/file_splitter.c

TRANSFER_SRC = src/transfer/transfer.c

COMMON_OBJ = $(COMMON_SRC:.c=.o)
STORAGE_OBJ = $(STORAGE_SRC:.c=.o)
TRANSFER_OBJ = $(TRANSFER_SRC:.c=.o)

ALL_OBJ = \
	$(COMMON_OBJ) \
	$(STORAGE_OBJ) \
	$(TRANSFER_OBJ)

all: $(ALL_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(ALL_OBJ)

.PHONY: all clean
