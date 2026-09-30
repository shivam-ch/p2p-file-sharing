#include "../src/common/protocol.h"
#include "../src/network/peer.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    int server_fd;
    int peer_fd;

    MessageType message_type;
    uint32_t received_size;
    char received_message[100];

    const char *response = "HELLO FROM PEER A";

    server_fd = start_listener(5001);

    if (server_fd < 0) {
        printf("Failed to start listener.\n");
        return 1;
    }

    printf("Peer A is listening on port 5001.\n");
    printf("Waiting for Peer B...\n");

    peer_fd = accept_peer(server_fd);

    if (peer_fd < 0) {
        printf("Failed to accept Peer B.\n");
        close(server_fd);
        return 1;
    }

    printf("Peer B connected successfully.\n");

    if (receive_message(
            peer_fd,
            &message_type,
            received_message,
            &received_size
        ) < 0) {
        printf("Failed to receive HELLO.\n");
        close(peer_fd);
        close(server_fd);
        return 1;
    }

    if (received_size >= sizeof(received_message)) {
        printf("Received message is too large.\n");
        close(peer_fd);
        return 1;
    }

    received_message[received_size] = '\0';

    printf("Received message type: %d\n", message_type);
    printf("Received message: %s\n", received_message);

    if (send_message(
            peer_fd,
            MSG_HELLO,
            response,
            (uint32_t)(strlen(response) + 1)
        ) < 0) {
        printf("Failed to send response.\n");
        close(peer_fd);
        close(server_fd);
        return 1;
    }

    printf("Peer A sent HELLO response.\n");

    close(peer_fd);
    close(server_fd);

    return 0;
}