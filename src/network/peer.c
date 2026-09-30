#include "peer.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int start_listener(uint16_t port)
{
    int server_fd;
    struct sockaddr_in server_address;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&server_address, 0, sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(port);

    if (bind(
            server_fd,
            (struct sockaddr *)&server_address,
            sizeof(server_address)
        ) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int connect_to_peer(const char *ip, uint16_t port)
{
    int socket_fd;
    struct sockaddr_in peer_address;

    socket_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (socket_fd < 0) {
        perror("socket");
        return -1;
    }

    memset(&peer_address, 0, sizeof(peer_address));

    peer_address.sin_family = AF_INET;
    peer_address.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &peer_address.sin_addr) <= 0) {
        perror("inet_pton");
        close(socket_fd);
        return -1;
    }

    if (connect(
            socket_fd,
            (struct sockaddr *)&peer_address,
            sizeof(peer_address)
        ) < 0) {
        perror("connect");
        close(socket_fd);
        return -1;
    }

    return socket_fd;
}

int accept_peer(int server_fd)
{
    int peer_fd;
    struct sockaddr_in peer_address;
    socklen_t address_length = sizeof(peer_address);

    peer_fd = accept(
        server_fd,
        (struct sockaddr *)&peer_address,
        &address_length
    );

    if (peer_fd < 0) {
        perror("accept");
        return -1;
    }

    return peer_fd;
}