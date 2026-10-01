#include "../src/network/peer.h"
#include "../src/storage/availability.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int server_fd;
    int peer_fd;

    server_fd = start_listener(5001);

    if (server_fd < 0) {
        printf("Failed to start Peer A.\n");
        return 1;
    }

    printf("Peer A listening on port 5001.\n");
    printf("Waiting for Peer B...\n");

    peer_fd = accept_peer(server_fd);

    if (peer_fd < 0) {
        printf("Failed to accept Peer B.\n");
        close(server_fd);
        return 1;
    }

    printf("Peer B connected.\n");

    if (send_piece_info(
            peer_fd,
            5,
            1
        ) != 0) {

        printf("Failed to send piece info.\n");
        close(peer_fd);
        close(server_fd);
        return 1;
    }

    printf(
        "Sent piece information: Piece 5 -> Peer 1\n"
    );

    close(peer_fd);
    close(server_fd);

    return 0;
}