#include "../src/network/peer.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int server_fd;
    int peer_fd;

    server_fd = start_listener(5003);

    if (server_fd < 0) {
        printf("Peer C failed to start.\n");
        return 1;
    }

    printf("Peer C listening on port 5003.\n");
    printf("Waiting for Peer B...\n");

    peer_fd = accept_peer(server_fd);

    if (peer_fd < 0) {
        printf("Failed to accept Peer B.\n");
        close(server_fd);
        return 1;
    }

    printf("Peer B connected directly to Peer C.\n");

    close(peer_fd);
    close(server_fd);

    return 0;
}