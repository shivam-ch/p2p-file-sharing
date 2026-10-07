#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

#include "../src/common/types.h"
#include "../src/network/peer.h"
#include "../src/network/discovery.h"
#include "../src/storage/availability.h"
#include "../src/storage/sha256.h"
#include "../src/transfer/transfer.h"

#define TEST_PORT 5101
#define PIECE_ID  0

typedef struct {
    int server_fd;
    int file_fd;
} ServerContext;

static void *upload_server(void *arg)
{
    ServerContext *context = arg;

    int client_fd = accept_peer(context->server_fd);

    if (client_fd < 0) {
        return NULL;
    }

    UploadTask task;

    task.socket_fd = client_fd;
    task.file_fd = context->file_fd;

    upload_worker(&task);

    close(client_fd);

    return NULL;
}

int main(void)
{
    const char *source_path = "tests/test_source.dat";
    const char *pieces_dir = "tests/test_peer_aware_pieces";

    const unsigned char test_data[] =
        "P2P peer-aware download integration test.";

    size_t data_size = sizeof(test_data) - 1;

    unsigned char hash[SHA256_HASH_SIZE];

    int source_fd;
    int server_fd;

    pthread_t server_thread;

    AvailabilityTable availability;
    PeerTable peers;
    TransferPiece piece;

    PeerInfo peer;

    printf("Starting peer-aware download integration test...\n");

    /*
     * --------------------------------------------------
     * 1. Create the source file owned by Peer 1
     * --------------------------------------------------
     */

    source_fd = open(
        source_path,
        O_CREAT | O_TRUNC | O_RDWR,
        0644
    );

    if (source_fd < 0) {
        perror("open source file");
        return 1;
    }

    if (write(source_fd, test_data, data_size) !=
        (ssize_t)data_size) {
        perror("write source file");
        close(source_fd);
        return 1;
    }

    /*
     * Calculate the SHA-256 hash expected for Piece 0.
     */

    if (calculate_sha256(
            source_path,
            0,
            data_size,
            hash
        ) != 0) {
        printf("FAIL: Could not calculate SHA-256\n");
        close(source_fd);
        return 1;
    }

    /*
     * --------------------------------------------------
     * 2. Start Peer 1's TCP listener
     * --------------------------------------------------
     */

    server_fd = start_listener(TEST_PORT);

    if (server_fd < 0) {
        printf("FAIL: Could not start peer listener\n");
        close(source_fd);
        return 1;
    }

    ServerContext server_context;

    server_context.server_fd = server_fd;
    server_context.file_fd = source_fd;

    if (pthread_create(
            &server_thread,
            NULL,
            upload_server,
            &server_context
        ) != 0) {
        printf("FAIL: Could not create server thread\n");
        close(server_fd);
        close(source_fd);
        return 1;
    }

    /*
     * --------------------------------------------------
     * 3. Create Peer B's availability information
     * --------------------------------------------------
     */

    availability_init(&availability);

    if (availability_add_piece(
            &availability,
            PIECE_ID
        ) != 0) {
        printf("FAIL: Could not add piece\n");
        return 1;
    }

    /*
     * Peer 1 owns Piece 0.
     */

    if (availability_add_peer(
            &availability,
            PIECE_ID,
            1
        ) != 0) {
        printf("FAIL: Could not add peer to availability\n");
        return 1;
    }

    /*
     * --------------------------------------------------
     * 4. Add Peer 1 to Peer B's peer table
     * --------------------------------------------------
     */

    peer.peer_id = 1;
    strcpy(peer.ip, "127.0.0.1");
    peer.port = TEST_PORT;

    peer_table_init(&peers);

    if (peer_table_add(&peers, peer) != 0) {
        printf("FAIL: Could not add peer\n");
        return 1;
    }

    /*
     * --------------------------------------------------
     * 5. Build the piece B wants to download
     * --------------------------------------------------
     */

    memset(&piece, 0, sizeof(piece));

    piece.info.piece_id = PIECE_ID;
    piece.info.offset = 0;
    piece.info.size = data_size;

    memcpy(
        piece.info.hash,
        hash,
        SHA256_HASH_SIZE
    );

    piece.info.status = PIECE_MISSING;
    piece.state = TRANSFER_MISSING;

    pthread_mutex_init(
        &piece.mutex,
        NULL
    );

    /*
     * --------------------------------------------------
     * 6. Use the actual peer-aware download path
     * --------------------------------------------------
     */

    printf(
        "Requesting Piece %u through availability table...\n",
        PIECE_ID
    );

    if (download_piece_from_peer(
            &availability,
            &peers,
            PIECE_ID,
            &piece,
            pieces_dir
        ) != 0) {
        printf("FAIL: Peer-aware download failed\n");

        pthread_join(server_thread, NULL);
        close(server_fd);
        close(source_fd);

        return 1;
    }

    /*
     * --------------------------------------------------
     * 7. Verify the resulting piece
     * --------------------------------------------------
     */

    if (piece.info.status != PIECE_AVAILABLE) {
        printf("FAIL: Piece was not marked available\n");

        pthread_join(server_thread, NULL);
        close(server_fd);
        close(source_fd);

        return 1;
    }

    printf("Piece downloaded successfully.\n");
    printf("SHA-256 verification passed.\n");

    /*
     * --------------------------------------------------
     * 8. Cleanup
     * --------------------------------------------------
     */

    pthread_join(server_thread, NULL);

    close(server_fd);
    close(source_fd);

    pthread_mutex_destroy(&piece.mutex);

    remove(source_path);

    char piece_path[256];

    snprintf(
        piece_path,
        sizeof(piece_path),
        "%s/piece_%u",
        pieces_dir,
        PIECE_ID
    );

    remove(piece_path);
    rmdir(pieces_dir);

    availability_free(&availability);

    printf("\nPeer-aware download integration test passed.\n");

    return 0;
}