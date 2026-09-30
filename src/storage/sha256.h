#ifndef SHA256_H
#define SHA256_H

#include <stdint.h>

#define SHA256_HASH_SIZE 32

int calculate_sha256(
    const char *filename,
    uint64_t offset,
    uint64_t size,
    unsigned char hash[SHA256_HASH_SIZE]
);

#endif
