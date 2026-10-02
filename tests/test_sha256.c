#include "sha256.h"

#include <stdio.h>
#include <string.h>

static void print_hash(
    const unsigned char hash[SHA256_HASH_SIZE]
)
{
    for (int i = 0; i < SHA256_HASH_SIZE; i++) {
        printf("%02x", hash[i]);
    }

    printf("\n");
}

int main(void)
{
    const char *filename = "tests/test_pieces.txt";

    unsigned char hash[SHA256_HASH_SIZE];

    /*
     * Calculate the SHA-256 hash of "BBBBB".
     *
     * test_pieces.txt contains:
     * AAAAABBBBBCCCCC
     *
     * Offset 5, size 5 selects "BBBBB".
     */
    if (calculate_sha256(
            filename,
            5,
            5,
            hash
        ) != 0) {

        fprintf(stderr, "SHA-256 calculation failed\n");
        return 1;
    }

    printf("SHA-256: ");
    print_hash(hash);

    /*
     * Verify the correct hash.
     */
    int result = verify_sha256(
        filename,
        5,
        5,
        hash
    );

    if (result != 1) {
        fprintf(
            stderr,
            "Correct hash verification failed\n"
        );

        return 1;
    }

    printf("Correct hash verification passed\n");

    /*
     * Create an intentionally incorrect hash.
     */
    unsigned char wrong_hash[SHA256_HASH_SIZE];

    memcpy(
        wrong_hash,
        hash,
        SHA256_HASH_SIZE
    );

    wrong_hash[0] ^= 0xFF;

    /*
     * Verify that the incorrect hash is rejected.
     */
    result = verify_sha256(
        filename,
        5,
        5,
        wrong_hash
    );

    if (result != 0) {
        fprintf(
            stderr,
            "Incorrect hash verification failed\n"
        );

        return 1;
    }

    printf("Incorrect hash rejection passed\n");

    /*
     * Test invalid input.
     */
    result = verify_sha256(
        NULL,
        5,
        5,
        hash
    );

    if (result != -1) {
        fprintf(
            stderr,
            "Invalid input test failed\n"
        );

        return 1;
    }

    printf("Invalid input test passed\n");

    printf("SHA-256 verification tests passed\n");

    return 0;
}
