#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

#define SHA256_HASH_SIZE 32

typedef struct {
    uint32_t piece_id;
    uint64_t offset;
    uint64_t size;
    unsigned char hash[SHA256_HASH_SIZE];
} PieceInfo;

typedef struct {
    uint32_t peer_id;
    char ip[46];
    uint16_t port;
} PeerInfo;

#endif