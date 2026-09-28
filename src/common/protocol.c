#include "protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

int send_all(int socket_fd, const void *buffer, size_t length)
{
    const char *data = buffer;
    size_t total_sent = 0;

    while (total_sent < length) {
        ssize_t sent = send(
            socket_fd,
            data + total_sent,
            length - total_sent,
            0
        );

        if (sent <= 0) {
            return -1;
        }

        total_sent += (size_t)sent;
    }

    return 0;
}

int receive_all(int socket_fd, void *buffer, size_t length)
{
    char *data = buffer;
    size_t total_received = 0;

    while (total_received < length) {
        ssize_t received = recv(
            socket_fd,
            data + total_received,
            length - total_received,
            0
        );

        if (received <= 0) {
            return -1;
        }

        total_received += (size_t)received;
    }

    return 0;
}

int send_message(
    int socket_fd,
    MessageType type,
    const void *payload,
    uint32_t payload_size
)
{
    MessageHeader header;

    header.type = htonl((uint32_t)type);
    header.payload_size = htonl(payload_size);

    if (send_all(socket_fd, &header, sizeof(header)) < 0) {
        return -1;
    }

    if (payload_size > 0 && payload != NULL) {
        if (send_all(socket_fd, payload, payload_size) < 0) {
            return -1;
        }
    }

    return 0;
}

int receive_message(
    int socket_fd,
    MessageType *type,
    void *payload,
    uint32_t payload_size
)
{
    MessageHeader header;

    if (receive_all(socket_fd, &header, sizeof(header)) < 0) {
        return -1;
    }

    header.type = ntohl(header.type);
    header.payload_size = ntohl(header.payload_size);

    if (type != NULL) {
        *type = (MessageType)header.type;
    }

    if (header.payload_size != payload_size) {
        return -1;
    }

    if (payload_size > 0 && payload != NULL) {
        if (receive_all(socket_fd, payload, payload_size) < 0) {
            return -1;
        }
    }

    return 0;
}