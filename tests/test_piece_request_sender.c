#include <stdio.h>
#include "../src/network/peer.h"
#include "../src/transfer/transfer.h"

int main(void)
{
    int socket_fd;
    PieceRequest request;

    socket_fd = connect_to_peer("127.0.0.1", 5001);

    if (socket_fd < 0) {
        printf("Failed to connect to peer.\n");
        return 1;
    }

    request.piece_id = 5;
    request.offset = 5242880ULL;
    request.size = 1048576ULL;

    printf("Sending piece request:\n");
    printf("  Piece ID: %u\n", request.piece_id);
    printf("  Offset: %llu\n",
           (unsigned long long)request.offset);
    printf("  Size: %llu\n",
           (unsigned long long)request.size);

    if (send_piece_request(socket_fd, &request) != 0) {
        printf("Failed to send piece request.\n");
        return 1;
    }

    printf("Piece request sent successfully.\n");

    return 0;
}