CC = gcc

CFLAGS = -Wall -Wextra -std=c11 \
         -I./src/common \
         -I./src/network \
         -I./src/storage \
         -I./src/transfer \
         -I./src/recovery \
         -I./src/app

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
	src/recovery/fairness.c \
	src/recovery/recovery.c \
	src/recovery/recovery_transfer.c

TRANSFER_SRC = \
	src/transfer/transfer.c

APP_SRC = \
	src/app/p2p.c

ALL_SRC = \
	$(COMMON_SRC) \
	$(NETWORK_SRC) \
	$(STORAGE_SRC) \
	$(RECOVERY_SRC) \
	$(TRANSFER_SRC)

ALL_OBJ = $(ALL_SRC:.c=.o)

APP_SRC = \
	src/app/p2p.c \
	src/app/main.c

P2P_SRC = \
	$(APP_SRC) \
	$(COMMON_SRC) \
	$(NETWORK_SRC) \
	$(STORAGE_SRC)

p2p: $(P2P_SRC)
	$(CC) $(CFLAGS) $(P2P_SRC) $(LDFLAGS) -o p2p

TEST_DIR = tests

.PHONY: all clean test core-tests

all: $(ALL_OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build and run the core unit/integration tests

test: core-tests
	@echo "========================================"
	@echo "Running complete test suite"
	@echo "========================================"
	@for test in \
		test_mid_transfer_dropout \
		test_three_peer_download \
		test_protocol \
		test_file_io \
		test_piece \
		test_piece_block \
		test_transfer \
		test_recovery \
		test_timeout \
		test_fairness \
		test_availability \
		test_tcp_transfer \
		test_tcp_concurrent_transfer \
		test_peer_aware_download \
		test_recovery_transfer \
		test_corruption_recovery; \
	do \
		echo "========================================"; \
		echo "Running $$test"; \
		echo "========================================"; \
		./tests/$$test || exit 1; \
	done
	@echo "========================================"
	@echo "ALL TESTS PASSED"
	@echo "========================================"

core-tests: test_mid_transfer_dropout test_three_peer_download test_protocol test_file_io test_piece test_piece_block test_transfer test_recovery test_timeout test_fairness test_availability test_tcp_transfer test_tcp_concurrent_transfer test_peer_aware_download test_recovery_transfer test_corruption_recovery

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

test_piece:
	$(CC) $(CFLAGS) \
		tests/test_piece.c \
		src/storage/piece.c \
		src/storage/sha256.c \
		$(LDFLAGS) \
		-o tests/test_piece

test_piece_block:
	$(CC) $(CFLAGS) \
		tests/test_piece_block.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/piece.c \
		src/storage/sha256.c \
		src/storage/file_io.c \
		src/storage/availability.c \
		src/network/discovery.c \
		src/network/peer.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
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
		src/network/discovery.c \
		src/network/peer.c \
		src/storage/availability.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		$(LDFLAGS) \
		-o tests/test_transfer

test_recovery:
	$(CC) $(CFLAGS) \
		tests/test_recovery.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		-o tests/test_recovery
test_timeout:
	$(CC) $(CFLAGS) \
		tests/test_timeout.c \
		src/recovery/timeout.c \
		-o tests/test_timeout

test_fairness:
	$(CC) $(CFLAGS) \
		tests/test_fairness.c \
		src/recovery/fairness.c \
		-o tests/test_fairness

test_availability:
	gcc -Wall -Wextra -std=c11 \
	    -I./src/common -I./src/storage \
	    tests/test_availability.c \
	    src/storage/availability.c \
	    src/common/protocol.c \
	    -o tests/test_availability

test_tcp_transfer:
	$(CC) $(CFLAGS) \
		tests/test_tcp_transfer.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/file_io.c \
		src/storage/sha256.c \
		src/network/peer.c \
		src/network/discovery.c \
		src/storage/availability.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		$(LDFLAGS) \
		-o tests/test_tcp_transfer

test_tcp_concurrent_transfer:
	$(CC) $(CFLAGS) \
		tests/test_tcp_concurrent_transfer.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/file_io.c \
		src/storage/sha256.c \
		src/network/peer.c \
		src/network/discovery.c \
		src/storage/availability.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		$(LDFLAGS) \
		-o tests/test_tcp_concurrent_transfer

test_peer_aware_download:
	$(CC) $(CFLAGS) \
		tests/test_peer_aware_download.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/file_io.c \
		src/storage/sha256.c \
		src/storage/piece.c \
		src/storage/availability.c \
		src/network/peer.c \
		src/network/discovery.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		$(LDFLAGS) \
		-o tests/test_peer_aware_download

test_three_peer_download:
	$(CC) $(CFLAGS) \
		tests/test_three_peer_download.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/file_io.c \
		src/storage/sha256.c \
		src/storage/piece.c \
		src/storage/file_splitter.c \
		src/storage/availability.c \
		src/network/peer.c \
		src/network/discovery.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		src/recovery/fairness.c \
		$(LDFLAGS) \
		-o tests/test_three_peer_download

test_mid_transfer_dropout:
	$(CC) $(CFLAGS) \
		tests/test_mid_transfer_dropout.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/file_io.c \
		src/storage/sha256.c \
		src/storage/piece.c \
		src/storage/file_splitter.c \
		src/storage/availability.c \
		src/network/peer.c \
		src/network/discovery.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		src/recovery/fairness.c \
		$(LDFLAGS) \
		-o tests/test_mid_transfer_dropout

test_recovery_transfer:
	$(CC) $(CFLAGS) \
		tests/test_recovery_transfer.c \
		src/recovery/recovery_transfer.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		src/recovery/fairness.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/piece.c \
		src/storage/sha256.c \
		src/storage/file_io.c \
		src/storage/availability.c \
		src/network/peer.c \
		src/network/discovery.c \
		$(LDFLAGS) \
		-o tests/test_recovery_transfer

test_corruption_recovery:
	$(CC) $(CFLAGS) \
		tests/test_corruption_recovery.c \
		src/recovery/recovery_transfer.c \
		src/recovery/recovery.c \
		src/recovery/failure.c \
		src/recovery/fairness.c \
		src/transfer/transfer.c \
		src/common/protocol.c \
		src/storage/piece.c \
		src/storage/sha256.c \
		src/storage/file_io.c \
		src/storage/availability.c \
		src/network/peer.c \
		src/network/discovery.c \
		$(LDFLAGS) \
		-o tests/test_corruption_recovery

clean:
	rm -f $(ALL_OBJ)
	rm -f tests/test_protocol
	rm -f tests/test_file_io
	rm -f tests/test_piece
	rm -f tests/test_piece_block
	rm -f tests/test_transfer
	rm -f tests/test_recovery
	rm -f tests/test_timeout
	rm -f tests/test_fairness
	rm -f tests/test_availability
	rm -f tests/test_tcp_transfer
	rm -f tests/test_tcp_concurrent_transfer
	rm -f tests/test_peer_aware_download
	rm -f tests/test_recovery_transfer
	rm -f tests/test_corruption_recovery
	rm -f tests/test_three_peer_download
	rm -f tests/test_mid_transfer_dropout


