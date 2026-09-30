#include "sha256.h"

#include <openssl/sha.h>
#include <stdio.h>

#define SHA256_BUFFER_SIZE (64 * 1024)

int calculate_sha256(
    const char *filename,
    uint64_t offset,
    uint64_t size,
    unsigned char hash[SHA256_HASH_SIZE]
)
{
    FILE *file;
    SHA256_CTX context;
    unsigned char buffer[SHA256_BUFFER_SIZE];

    file = fopen(filename, "rb");

    if (file == NULL) {
        return -1;
    }

    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    if (SHA256_Init(&context) != 1) {
        fclose(file);
        return -1;
    }

    while (size > 0) {
        size_t bytes_to_read;

        if (size > SHA256_BUFFER_SIZE) {
            bytes_to_read = SHA256_BUFFER_SIZE;
        } else {
            bytes_to_read = (size_t)size;
        }

        size_t bytes_read = fread(
            buffer,
            1,
            bytes_to_read,
            file
        );

        if (bytes_read != bytes_to_read) {
            fclose(file);
            return -1;
        }

        if (SHA256_Update(&context, buffer, bytes_read) != 1) {
            fclose(file);
            return -1;
        }

        size -= bytes_read;
    }

    if (SHA256_Final(hash, &context) != 1) {
        fclose(file);
        return -1;
    }

    fclose(file);

    return 0;
}
