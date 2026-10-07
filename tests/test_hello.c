#include "../src/app/p2p.h"
#include "../src/network/peer.h"

#include <stdio.h>
#include <unistd.h>

int main(void)
{
    int server_fd;
    int client_fd;
    int peer_fd;

    uint32_t received_peer_id;
    uint16_t received_port;

    server_fd = start_listener(19500);

    if (server_fd < 0) {
        printf("Failed to start listener.\n");
        return 1;
    }

    client_fd = connect_to_peer("127.0.0.1", 19500);

    if (client_fd < 0) {
        printf("Failed to connect to listener.\n");
        close(server_fd);
        return 1;
    }

    peer_fd = accept_peer(server_fd);

    if (peer_fd < 0) {
        printf("Failed to accept peer.\n");
        close(client_fd);
        close(server_fd);
        return 1;
    }

    /* Client sends HELLO */
    if (p2p_send_hello(client_fd, 42, 5042) != 0) {
        printf("Failed to send HELLO.\n");
        close(peer_fd);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    /* Server receives HELLO */
    if (p2p_receive_hello(
            peer_fd,
            &received_peer_id,
            &received_port
        ) != 0) {

        printf("Failed to receive HELLO.\n");
        close(peer_fd);
        close(client_fd);
        close(server_fd);
        return 1;
    }

    printf(
        "Received HELLO: Peer ID=%u Port=%u\n",
        received_peer_id,
        received_port
    );

    if (received_peer_id != 42 ||
        received_port != 5042) {

        printf("HELLO data verification failed.\n");

        close(peer_fd);
        close(client_fd);
        close(server_fd);

        return 1;
    }

    close(peer_fd);
    close(client_fd);
    close(server_fd);

    printf("HELLO protocol test passed.\n");

    return 0;
}
