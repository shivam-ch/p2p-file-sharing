#include "../src/network/peer.h"
#include "../src/network/discovery.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int peer_a_fd;
    int peer_c_fd;

    PeerTable table;

    printf("Peer B connecting to Peer A...\n");

    peer_a_fd = connect_to_peer("127.0.0.1", 5001);

    if (peer_a_fd < 0) {
        printf("Failed to connect to Peer A.\n");
        return 1;
    }

    printf("Connected to Peer A.\n");

    if (receive_peer_list(peer_a_fd, &table) != 0) {
        printf("Failed to receive peer list.\n");
        close(peer_a_fd);
        return 1;
    }

    printf(
        "Peer B received %zu peers from Peer A.\n",
        table.count
    );

    PeerInfo *peer_c = peer_table_find(&table, 3);

    if (peer_c == NULL) {
        printf("Peer C was not found in peer list.\n");
        close(peer_a_fd);
        return 1;
    }

    printf(
        "Discovered Peer C: %s:%u\n",
        peer_c->ip,
        peer_c->port
    );

    peer_c_fd = connect_to_peer(
        peer_c->ip,
        peer_c->port
    );

    if (peer_c_fd < 0) {
        printf("Failed to connect directly to Peer C.\n");
        close(peer_a_fd);
        return 1;
    }

    printf("Peer B connected directly to Peer C.\n");

    close(peer_c_fd);
    close(peer_a_fd);

    return 0;
}