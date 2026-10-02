#include <stdio.h>
#include <string.h>

#include "../src/network/peer.h"
#include "../src/transfer/transfer.h"

int main(void)
{
    int socket_fd;
    PieceBlock block;
    const unsigned char data[] = "HELLO PIECE BLOCK";

    socket_fd = connect_to_peer("127.0.0.1", 5001);

    if (socket_fd < 0) {
        printf("Failed to connect to peer.\n");
        return 1;
    }

    block.piece_id = 5;
    block.offset = 5242880ULL;
    block.data_size = (uint32_t)strlen((const char *)data);

    printf("Sending piece block:\n");
    printf("  Piece ID: %u\n", block.piece_id);
    printf("  Offset: %llu\n",
           (unsigned long long)block.offset);
    printf("  Data size: %u\n", block.data_size);
    printf("  Data: %s\n", data);

    if (send_piece_block(socket_fd, &block, data) != 0) {
        printf("Failed to send piece block.\n");
        return 1;
    }

    printf("Piece block sent successfully.\n");

    return 0;
}