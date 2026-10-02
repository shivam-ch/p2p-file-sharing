CC = gcc

CFLAGS = -Wall -Wextra -std=c11 \
         -I./src/common \
         -I./src/network \
         -I./src/storage \
         -I./src/transfer \
         -I./src/recovery

LDFLAGS = -pthread -lcrypto

COMMON_SRC = \
	src/common/protocol.c

NETWORK_SRC = \
	src/network/peer.c \
	src/network/discovery.c

STORAGE_SRC = \
	src/storage/piece.c \
	src/storage/sha256.c \
	src/storage/file_splitter.c \
	src/storage/availability.c \
	src/storage/file_io.c

RECOVERY_SRC = \
	src/recovery/failure.c \
	src/recovery/timeout.c \
	src/recovery/fairness.c

TRANSFER_SRC = \
	src/transfer/transfer.c

ALL_SRC = \
	$(COMMON_SRC) \
	$(NETWORK_SRC) \
	$(STORAGE_SRC) \
	$(RECOVERY_SRC) \
	$(TRANSFER_SRC)

ALL_OBJ = $(ALL_SRC:.c=.o)

TEST_DIR = tests

.PHONY: all clean test core-tests

all: $(ALL_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run the core unit/integration tests
test: core-tests
	@echo "All selected tests built successfully."

core-tests: \
	test_protocol \
	test_file_io \
	test_piece_block \
	test_transfer

test_protocol:
	$(CC) $(CFLAGS) \
		tests/test_protocol.c \
		src/common/protocol.c \
		$(LDFLAGS) \
		-o tests/test_protocol

test_file_io:
	$(CC) $(CFLAGS) \
		tests/test_file_io.c \
		src/storage/file_io.c \
		-o tests/test_file_io

test_piece_block:
	$(CC) $(CFLAGS) \
		tests/test_piece_block.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/piece.c \
		src/storage/sha256.c \
		src/storage/file_io.c \
		$(LDFLAGS) \
		-o tests/test_piece_block

test_transfer:
	$(CC) $(CFLAGS) \
		tests/test_transfer.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/piece.c \
		src/storage/sha256.c \
		src/storage/file_io.c \
		$(LDFLAGS) \
		-o tests/test_transfer

clean:
	rm -f $(ALL_OBJ)
	rm -f tests/test_protocol
	rm -f tests/test_file_io
	rm -f tests/test_piece_block
	rm -f tests/test_transfer

.PHONY: all clean test core-tests