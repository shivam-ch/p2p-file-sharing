#include <stdio.h>

#include "../src/network/peer.h"
#include "../src/transfer/transfer.h"

int main(void)
{
    int server_fd;
    int client_fd;

    PieceBlock block;
    unsigned char data[MAX_PIECE_BLOCK_DATA + 1];

    server_fd = start_listener(5001);

    if (server_fd < 0) {
        printf("Failed to start listener.\n");
        return 1;
    }

    printf("Waiting for peer on port 5001...\n");

    client_fd = accept_peer(server_fd);

    if (client_fd < 0) {
        printf("Failed to accept peer.\n");
        return 1;
    }

    if (receive_piece_block(
            client_fd,
            &block,
            data,
            MAX_PIECE_BLOCK_DATA
        ) != 0) {
        printf("Failed to receive piece block.\n");
        return 1;
    }

    data[block.data_size] = '\0';

    printf("Received piece block:\n");
    printf("  Piece ID: %u\n", block.piece_id);
    printf("  Offset: %llu\n",
           (unsigned long long)block.offset);
    printf("  Data size: %u\n", block.data_size);
    printf("  Data: %s\n", data);

    return 0;
}