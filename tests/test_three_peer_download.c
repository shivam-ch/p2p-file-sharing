#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "../src/common/types.h"
#include "../src/network/peer.h"
#include "../src/network/discovery.h"
#include "../src/storage/file_splitter.h"
#include "../src/storage/availability.h"
#include "../src/storage/sha256.h"
#include "../src/transfer/transfer.h"

#define TEST_PEER_COUNT 3
#define TEST_PIECE_COUNT 3
#define PIECE_SIZE 100000

#define PEER1_PORT 19101
#define PEER2_PORT 19102
#define PEER3_PORT 19103

#define TEST_FILE "/tmp/p2p_three_peer_source.bin"
#define SPLIT_DIR "/tmp/p2p_three_peer_source_pieces"
#define DOWNLOAD_DIR "/tmp/p2p_three_peer_download"
#define RECONSTRUCTED_FILE "/tmp/p2p_three_peer_reconstructed.bin"


typedef struct
{
    int server_fd;
    int file_fd;
} ServerContext;


typedef struct
{
    AvailabilityTable *availability;
    PeerTable *peers;

    uint32_t piece_id;
    TransferPiece *piece;

    const char *pieces_dir;

    int result;
} DownloadContext;


/*
 * One serving peer accepts one downloader connection.
 */
static void *server_thread(void *arg)
{
    ServerContext *ctx = arg;

    int client_fd = accept_peer(ctx->server_fd);

    if (client_fd < 0) {
        perror("accept_peer");
        return NULL;
    }

    UploadTask upload;

    upload.socket_fd = client_fd;
    upload.file_fd = ctx->file_fd;

    upload_worker(&upload);

    close(client_fd);

    return NULL;
}


/*
 * Download one piece through the availability table.
 */
static void *download_thread(void *arg)
{
    DownloadContext *ctx = arg;

    if (ctx == NULL) {
        return NULL;
    }

    printf(
        "Downloader requesting Piece %u...\n",
        ctx->piece_id
    );

    ctx->result = download_piece_from_peer(
        ctx->availability,
        ctx->peers,
        ctx->piece_id,
        ctx->piece,
        ctx->pieces_dir
    );

    if (ctx->result == 0) {
        printf(
            "Piece %u downloaded successfully.\n",
            ctx->piece_id
        );
    } else {
        printf(
            "Piece %u download failed.\n",
            ctx->piece_id
        );
    }

    return NULL;
}


/*
 * Create a deterministic source file.
 */
static int create_source_file(void)
{
    unsigned char buffer[PIECE_SIZE];

    int fd = open(
        TEST_FILE,
        O_CREAT | O_TRUNC | O_WRONLY,
        0644
    );

    if (fd < 0) {
        perror("open source file");
        return -1;
    }

    for (int piece = 0;
         piece < TEST_PIECE_COUNT;
         piece++) {

        for (size_t i = 0;
             i < sizeof(buffer);
             i++) {

            buffer[i] =
                (unsigned char)((piece * 31 + i) % 251);
        }

        if (write(
                fd,
                buffer,
                sizeof(buffer)
            ) != (ssize_t)sizeof(buffer)) {

            perror("write source file");
            close(fd);
            return -1;
        }
    }

    close(fd);

    return 0;
}


/*
 * Remove files created by the test.
 */
static void cleanup(void)
{
    char path[512];

    unlink(TEST_FILE);
    unlink(RECONSTRUCTED_FILE);

    for (int i = 0;
         i < TEST_PIECE_COUNT;
         i++) {

        snprintf(
            path,
            sizeof(path),
            "%s/piece_%d",
            SPLIT_DIR,
            i
        );

        unlink(path);

        snprintf(
            path,
            sizeof(path),
            "%s/piece_%d",
            DOWNLOAD_DIR,
            i
        );

        unlink(path);
    }

    rmdir(SPLIT_DIR);
    rmdir(DOWNLOAD_DIR);
}


int main(void)
{
    printf(
        "Starting 3-peer concurrent full-file download test...\n"
    );

    cleanup();

    /*
     * ---------------------------------------------------------
     * 1. Create source file
     * ---------------------------------------------------------
     */
    if (create_source_file() != 0) {
        return 1;
    }


    /*
     * ---------------------------------------------------------
     * 2. Split source file into three pieces
     * ---------------------------------------------------------
     */
    PieceInfo *pieces = NULL;
    size_t piece_count = 0;

    if (split_file(
            TEST_FILE,
            SPLIT_DIR,
            PIECE_SIZE,
            &pieces,
            &piece_count
        ) != 0) {

        fprintf(stderr, "split_file failed\n");
        cleanup();
        return 1;
    }

    if (piece_count != TEST_PIECE_COUNT) {

        fprintf(
            stderr,
            "Expected %d pieces, got %zu\n",
            TEST_PIECE_COUNT,
            piece_count
        );

        free_pieces(pieces);
        cleanup();

        return 1;
    }

    printf(
        "Source file split into %zu pieces.\n",
        piece_count
    );


    /*
     * ---------------------------------------------------------
     * 3. Create downloader's piece directory
     * ---------------------------------------------------------
     */
    if (mkdir(DOWNLOAD_DIR, 0755) != 0) {

        perror("mkdir download directory");

        free_pieces(pieces);
        cleanup();

        return 1;
    }


    /*
     * ---------------------------------------------------------
     * 4. Start three serving peers
     * ---------------------------------------------------------
     */
    int ports[TEST_PEER_COUNT] = {
        PEER1_PORT,
        PEER2_PORT,
        PEER3_PORT
    };

    int server_fds[TEST_PEER_COUNT];

    for (int i = 0;
         i < TEST_PEER_COUNT;
         i++) {

        server_fds[i] = start_listener(
            (uint16_t)ports[i]
        );

        if (server_fds[i] < 0) {

            fprintf(
                stderr,
                "Failed to start Peer %d\n",
                i + 1
            );

            free_pieces(pieces);
            cleanup();

            return 1;
        }

        printf(
            "Peer %d listening on port %d\n",
            i + 1,
            ports[i]
        );
    }


    /*
     * ---------------------------------------------------------
     * 5. Open source file for every peer
     * ---------------------------------------------------------
     */
    int source_fds[TEST_PEER_COUNT];

    for (int i = 0;
         i < TEST_PEER_COUNT;
         i++) {

        source_fds[i] = open(
            TEST_FILE,
            O_RDONLY
        );

        if (source_fds[i] < 0) {

            perror("open source for peer");

            free_pieces(pieces);
            cleanup();

            return 1;
        }
    }


    /*
     * ---------------------------------------------------------
     * 6. Start server threads
     * ---------------------------------------------------------
     */
    ServerContext server_contexts[TEST_PEER_COUNT];
    pthread_t server_threads[TEST_PEER_COUNT];

    for (int i = 0;
         i < TEST_PEER_COUNT;
         i++) {

        server_contexts[i].server_fd =
            server_fds[i];

        server_contexts[i].file_fd =
            source_fds[i];

        if (pthread_create(
                &server_threads[i],
                NULL,
                server_thread,
                &server_contexts[i]
            ) != 0) {

            fprintf(
                stderr,
                "Failed to create server thread for Peer %d\n",
                i + 1
            );

            return 1;
        }
    }


    /*
     * ---------------------------------------------------------
     * 7. Build PeerTable
     * ---------------------------------------------------------
     */
    PeerTable peer_table;

    peer_table_init(&peer_table);

    for (int i = 0;
         i < TEST_PEER_COUNT;
         i++) {

        PeerInfo peer;

        memset(&peer, 0, sizeof(peer));

        peer.peer_id = (uint32_t)(i + 1);

        strcpy(
            peer.ip,
            "127.0.0.1"
        );

        peer.port =
            (uint16_t)ports[i];

        if (peer_table_add(
                &peer_table,
                peer
            ) != 0) {

            fprintf(
                stderr,
                "Failed to add Peer %d\n",
                i + 1
            );

            return 1;
        }
    }


    /*
     * ---------------------------------------------------------
     * 8. Build AvailabilityTable
     *
     * Piece 0 -> Peer 1
     * Piece 1 -> Peer 2
     * Piece 2 -> Peer 3
     * ---------------------------------------------------------
     */
    AvailabilityTable availability;

    availability_init(&availability);

    for (uint32_t piece_id = 0;
         piece_id < TEST_PIECE_COUNT;
         piece_id++) {

        if (availability_add_piece(
                &availability,
                piece_id
            ) != 0) {

            fprintf(
                stderr,
                "Failed to add Piece %u\n",
                piece_id
            );

            return 1;
        }

        if (availability_add_peer(
                &availability,
                piece_id,
                piece_id + 1
            ) != 0) {

            fprintf(
                stderr,
                "Failed to add Peer for Piece %u\n",
                piece_id
            );

            return 1;
        }
    }

    printf("\nPiece availability:\n");
    printf("  Piece 0 -> Peer 1\n");
    printf("  Piece 1 -> Peer 2\n");
    printf("  Piece 2 -> Peer 3\n\n");


    /*
     * ---------------------------------------------------------
     * 9. Create downloader TransferPiece objects
     * ---------------------------------------------------------
     */
    TransferPiece transfer_pieces[TEST_PIECE_COUNT];

    memset(
        transfer_pieces,
        0,
        sizeof(transfer_pieces)
    );

    for (int i = 0;
         i < TEST_PIECE_COUNT;
         i++) {

        transfer_pieces[i].info =
            pieces[i];

        transfer_pieces[i].state =
            TRANSFER_MISSING;

        if (pthread_mutex_init(
                &transfer_pieces[i].mutex,
                NULL
            ) != 0) {

            fprintf(
                stderr,
                "pthread_mutex_init failed for Piece %d\n",
                i
            );

            return 1;
        }
    }


    /*
     * ---------------------------------------------------------
     * 10. Start one downloader thread per piece
     * ---------------------------------------------------------
     */
    DownloadContext download_contexts[
        TEST_PIECE_COUNT
    ];

    pthread_t download_threads[
        TEST_PIECE_COUNT
    ];

    printf(
        "Starting concurrent downloads...\n\n"
    );

    for (int i = 0;
         i < TEST_PIECE_COUNT;
         i++) {

        download_contexts[i].availability =
            &availability;

        download_contexts[i].peers =
            &peer_table;

        download_contexts[i].piece_id =
            (uint32_t)i;

        download_contexts[i].piece =
            &transfer_pieces[i];

        download_contexts[i].pieces_dir =
            DOWNLOAD_DIR;

        download_contexts[i].result = -1;

        if (pthread_create(
                &download_threads[i],
                NULL,
                download_thread,
                &download_contexts[i]
            ) != 0) {

            fprintf(
                stderr,
                "Failed to create download thread for Piece %d\n",
                i
            );

            return 1;
        }
    }


    /*
     * ---------------------------------------------------------
     * 11. Wait for all downloads
     * ---------------------------------------------------------
     */
    int download_failed = 0;

    for (int i = 0;
         i < TEST_PIECE_COUNT;
         i++) {

        pthread_join(
            download_threads[i],
            NULL
        );

        if (download_contexts[i].result != 0) {
            download_failed = 1;
        }
    }

    if (download_failed) {

        fprintf(
            stderr,
            "One or more piece downloads failed.\n"
        );

        return 1;
    }

    printf(
        "\nAll three pieces downloaded successfully.\n"
    );


    /*
     * ---------------------------------------------------------
     * 12. Reconstruct complete file
     * ---------------------------------------------------------
     */
    if (reconstruct_file(
            RECONSTRUCTED_FILE,
            DOWNLOAD_DIR,
            pieces,
            piece_count
        ) != 0) {

        fprintf(
            stderr,
            "File reconstruction failed.\n"
        );

        return 1;
    }

    printf(
        "File reconstructed successfully.\n"
    );


    /*
     * ---------------------------------------------------------
     * 13. Verify final file using SHA-256
     * ---------------------------------------------------------
     */
    unsigned char original_hash[
        SHA256_HASH_SIZE
    ];

    unsigned char reconstructed_hash[
        SHA256_HASH_SIZE
    ];

    uint64_t file_size =
        (uint64_t)TEST_PIECE_COUNT * PIECE_SIZE;

    if (calculate_sha256(
            TEST_FILE,
            0,
            file_size,
            original_hash
        ) != 0) {

        fprintf(
            stderr,
            "Original SHA-256 calculation failed.\n"
        );

        return 1;
    }

    if (calculate_sha256(
            RECONSTRUCTED_FILE,
            0,
            file_size,
            reconstructed_hash
        ) != 0) {

        fprintf(
            stderr,
            "Reconstructed SHA-256 calculation failed.\n"
        );

        return 1;
    }

    if (memcmp(
            original_hash,
            reconstructed_hash,
            SHA256_HASH_SIZE
        ) != 0) {

        fprintf(
            stderr,
            "Final SHA-256 verification failed.\n"
        );

        return 1;
    }


    /*
     * ---------------------------------------------------------
     * 14. Cleanup
     * ---------------------------------------------------------
     */
    for (int i = 0;
         i < TEST_PEER_COUNT;
         i++) {

        close(server_fds[i]);

        pthread_join(
            server_threads[i],
            NULL
        );

        close(source_fds[i]);
    }

    for (int i = 0;
         i < TEST_PIECE_COUNT;
         i++) {

        pthread_mutex_destroy(
            &transfer_pieces[i].mutex
        );
    }

    /*
     * AvailabilityTable owns dynamically allocated
     * PieceAvailability storage.
     */
    availability_free(&availability);

    free_pieces(pieces);

    cleanup();


    /*
     * ---------------------------------------------------------
     * Final result
     * ---------------------------------------------------------
     */
    printf("\n");
    printf(
        "3-peer concurrent full-file download test passed.\n"
    );
    printf(
        "Three peers supplied pieces concurrently.\n"
    );
    printf(
        "File reconstruction passed.\n"
    );
    printf(
        "Final SHA-256 verification passed.\n"
    );

    return 0;
}