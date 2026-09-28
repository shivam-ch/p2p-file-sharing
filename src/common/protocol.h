#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    MSG_HELLO = 1,
    MSG_PEER_LIST,
    MSG_PIECE_INFO,
    MSG_PIECE_REQUEST,
    MSG_PIECE_RESPONSE,
    MSG_PING,
    MSG_PONG,
    MSG_DISCONNECT
} MessageType;

typedef struct {
    uint32_t type;
    uint32_t payload_size;
} MessageHeader;

int send_all(int socket_fd, const void *buffer, size_t length);

int receive_all(int socket_fd, void *buffer, size_t length);

int send_message(
    int socket_fd,
    MessageType type,
    const void *payload,
    uint32_t payload_size
);

int receive_message(
    int socket_fd,
    MessageType *type,
    void *payload,
    uint32_t payload_size
);

#endif