#include "../src/common/protocol.h"
#include "../src/network/peer.h"
#include "../src/network/discovery.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int server_fd;
    int peer_fd;

    PeerTable table;

    PeerInfo peer_b = {
        .peer_id = 2,
        .ip = "127.0.0.1",
        .port = 5002
    };

    PeerInfo peer_c = {
        .peer_id = 3,
        .ip = "127.0.0.1",
        .port = 5003
    };

    peer_table_init(&table);

    peer_table_add(&table, peer_b);
    peer_table_add(&table, peer_c);

    server_fd = start_listener(5001);

    if (server_fd < 0) {
        printf("Failed to start listener.\n");
        return 1;
    }

    printf("Peer A is listening on port 5001.\n");
    printf("Waiting for a peer...\n");

    peer_fd = accept_peer(server_fd);

    if (peer_fd < 0) {
        printf("Failed to accept peer.\n");
        close(server_fd);
        return 1;
    }

    printf("Peer connected.\n");

    if (send_peer_list(peer_fd, &table) != 0) {
        printf("Failed to send peer list.\n");
        close(peer_fd);
        close(server_fd);
        return 1;
    }

    printf("Peer list sent successfully.\n");

    close(peer_fd);
    close(server_fd);

    return 0;
}