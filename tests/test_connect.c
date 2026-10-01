#include "../src/common/protocol.h"
#include "../src/network/peer.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
    int peer_fd;
    const char *hello_message = "HELLO FROM PEER B";
    char received_message[100];
    MessageType message_type;
    uint32_t received_size;

    printf("Peer B is connecting to Peer A...\n");

    peer_fd = connect_to_peer("127.0.0.1", 5001);

    if (peer_fd < 0) {
        printf("Connection failed.\n");
        return 1;
    }

    printf("Peer B connected successfully.\n");

    if (send_message(
            peer_fd,
            MSG_HELLO,
            hello_message,
            (uint32_t)(strlen(hello_message) + 1)
        ) < 0) {
        printf("Failed to send HELLO.\n");
        close(peer_fd);
        return 1;
    }

    printf("Peer B sent HELLO.\n");

    if (receive_message(
        peer_fd,
        &message_type,
        received_message,
        sizeof(received_message),
        &received_size
        ) < 0) {
        printf("Failed to receive response.\n");
        close(peer_fd);
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

    close(peer_fd);

    return 0;
}