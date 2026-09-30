#define _FILE_OFFSET_BITS 64
#define _POSIX_C_SOURCE 200809L

#include "sha256.h"

#include <openssl/evp.h>
#include <stdio.h>
#include <sys/types.h>

#define SHA256_BUFFER_SIZE (64 * 1024)

int calculate_sha256(
    const char *filename,
    uint64_t offset,
    uint64_t size,
    unsigned char hash[SHA256_HASH_SIZE]
)
{
    FILE *file;
    EVP_MD_CTX *context;
    unsigned char buffer[SHA256_BUFFER_SIZE];
    unsigned int hash_length = 0;

    file = fopen(filename, "rb");

    if (file == NULL) {
        return -1;
    }

    if (fseeko(file, (off_t)offset, SEEK_SET) != 0) {
        fclose(file);
        return -1;
    }

    context = EVP_MD_CTX_new();

    if (context == NULL) {
        fclose(file);
        return -1;
    }

    if (EVP_DigestInit_ex(context, EVP_sha256(), NULL) != 1) {
        EVP_MD_CTX_free(context);
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
            EVP_MD_CTX_free(context);
            fclose(file);
            return -1;
        }

        if (EVP_DigestUpdate(context, buffer, bytes_read) != 1) {
            EVP_MD_CTX_free(context);
            fclose(file);
            return -1;
        }

        size -= bytes_read;
    }

    if (EVP_DigestFinal_ex(context, hash, &hash_length) != 1) {
        EVP_MD_CTX_free(context);
        fclose(file);
        return -1;
    }

    EVP_MD_CTX_free(context);
    fclose(file);

    if (hash_length != SHA256_HASH_SIZE) {
        return -1;
    }

    return 0;
}
