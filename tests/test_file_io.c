#include <stdio.h>
#include <string.h>

#include "../src/storage/file_io.h"

int main(void)
{
    const char *source_file = "test_source.bin";
    const char *destination_file = "test_destination.bin";

    const unsigned char original_data[] =
        "This is a test file for P2P piece transfer.";

    unsigned char read_buffer[100];
    unsigned char verify_buffer[100];

    FILE *file;

    size_t data_size = strlen((const char *)original_data);

    /* Create source file */
    file = fopen(source_file, "wb");

    if (file == NULL) {
        printf("Failed to create source file.\n");
        return 1;
    }

    fwrite(original_data, 1, data_size, file);
    fclose(file);

    /* Read data using our helper */
    if (read_piece_block(
            source_file,
            0,
            (uint32_t)data_size,
            read_buffer
        ) != 0) {
        printf("Failed to read piece block.\n");
        return 1;
    }

    printf("Read %zu bytes successfully.\n", data_size);

    /* Write data using our helper */
    if (write_piece_block(
            destination_file,
            0,
            (uint32_t)data_size,
            read_buffer
        ) != 0) {
        printf("Failed to write piece block.\n");
        return 1;
    }

    /* Read destination back for verification */
    if (read_piece_block(
            destination_file,
            0,
            (uint32_t)data_size,
            verify_buffer
        ) != 0) {
        printf("Failed to read destination file.\n");
        return 1;
    }

    if (memcmp(
            original_data,
            verify_buffer,
            data_size
        ) != 0) {
        printf("File I/O test FAILED.\n");
        return 1;
    }

    printf("File I/O test passed.\n");

    remove(source_file);
    remove(destination_file);

    return 0;
}