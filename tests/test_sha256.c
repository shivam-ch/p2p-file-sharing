#include "sha256.h"

#include <stdio.h>

static void print_hash(const unsigned char hash[SHA256_HASH_SIZE])
{
    for (int i = 0; i < SHA256_HASH_SIZE; i++) {
        printf("%02x", hash[i]);
    }

    printf("\n");
}

int main(void)
{
    unsigned char hash[SHA256_HASH_SIZE];

    if (calculate_sha256(
            "tests/test_pieces.txt",
            5,
            5,
            hash
        ) != 0) {

        fprintf(stderr, "SHA-256 calculation failed\n");
        return 1;
    }

    printf("SHA-256: ");
    print_hash(hash);

    return 0;
}
