#include <stdio.h>
#include "../src/network/peer.h"
#include "../src/transfer/transfer.h"

int main(void)
{
    int server_fd;
    int client_fd;
    PieceRequest request;

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

    if (receive_piece_request(client_fd, &request) != 0) {
        printf("Failed to receive piece request.\n");
        return 1;
    }

    printf("Received piece request:\n");
    printf("  Piece ID: %u\n", request.piece_id);
    printf("  Offset: %llu\n",
           (unsigned long long)request.offset);
    printf("  Size: %llu\n",
           (unsigned long long)request.size);

    return 0;
}