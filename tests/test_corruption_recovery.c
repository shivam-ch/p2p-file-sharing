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
#include "../src/storage/sha256.h"
#include "../src/transfer/transfer.h"
#include "../src/recovery/recovery_transfer.h"

#define BAD_PEER_PORT 19095
#define GOOD_PEER_PORT 19096

#define TEST_SIZE 100000
#define TEST_PIECES_DIR "/tmp/p2p_corruption_recovery_pieces"
#define GOOD_FILE "/tmp/p2p_good_source.bin"
#define BAD_FILE "/tmp/p2p_bad_source.bin"

typedef struct
{
    int server_fd;
    int file_fd;
} ServerContext;

static void *server_thread(void *arg)
{
    ServerContext *ctx = (ServerContext *)arg;

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

static int create_file(const char *path, int corrupt)
{
    unsigned char buffer[TEST_SIZE];

    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = (unsigned char)((i * 37) % 251);
    }

    if (corrupt) {
        buffer[12345] ^= 0xFF;
    }

    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);

    if (fd < 0) {
        perror("open source");
        return -1;
    }

    if (write(fd, buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer)) {
        perror("write source");
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

int main(void)
{
    printf("Starting corrupted-piece recovery test...\n");

    if (create_file(GOOD_FILE, 0) != 0 ||
        create_file(BAD_FILE, 1) != 0) {
        return 1;
    }

    system("rm -rf " TEST_PIECES_DIR);

    if (mkdir(TEST_PIECES_DIR, 0755) != 0) {
        perror("mkdir");
        return 1;
    }

    unsigned char expected_hash[SHA256_HASH_SIZE];

    if (calculate_sha256(
            GOOD_FILE,
            0,
            TEST_SIZE,
            expected_hash) != 0) {

        fprintf(stderr, "Failed to calculate expected SHA-256\n");
        return 1;
    }

    int bad_server = start_listener(BAD_PEER_PORT);
    int good_server = start_listener(GOOD_PEER_PORT);

    if (bad_server < 0 || good_server < 0) {
        fprintf(stderr, "Failed to start test listeners\n");
        return 1;
    }

    int bad_fd = open(BAD_FILE, O_RDONLY);
    int good_fd = open(GOOD_FILE, O_RDONLY);

    if (bad_fd < 0 || good_fd < 0) {
        perror("open source");
        return 1;
    }

    ServerContext bad_ctx = {
        .server_fd = bad_server,
        .file_fd = bad_fd
    };

    ServerContext good_ctx = {
        .server_fd = good_server,
        .file_fd = good_fd
    };

    pthread_t bad_thread;
    pthread_t good_thread;

    if (pthread_create(
            &bad_thread,
            NULL,
            server_thread,
            &bad_ctx) != 0) {
        fprintf(stderr, "Failed to create bad peer thread\n");
        return 1;
    }

    if (pthread_create(
            &good_thread,
            NULL,
            server_thread,
            &good_ctx) != 0) {
        fprintf(stderr, "Failed to create good peer thread\n");
        return 1;
    }

    TransferPiece piece;
    memset(&piece, 0, sizeof(piece));

    piece.info.piece_id = 0;
    piece.info.offset = 0;
    piece.info.size = TEST_SIZE;

    memcpy(
        piece.info.hash,
        expected_hash,
        SHA256_HASH_SIZE);

    piece.info.status = PIECE_MISSING;
    piece.state = TRANSFER_MISSING;

    if (pthread_mutex_init(&piece.mutex, NULL) != 0) {
        fprintf(stderr, "pthread_mutex_init failed\n");
        return 1;
    }

    FailureTable failure_table;
    FairnessTable fairness_table;

    failure_table_init(&failure_table);
    fairness_init(&fairness_table);

    PeerInfo peers[2];
    memset(peers, 0, sizeof(peers));

    peers[0].peer_id = 1;
    strcpy(peers[0].ip, "127.0.0.1");
    peers[0].port = BAD_PEER_PORT;

    peers[1].peer_id = 2;
    strcpy(peers[1].ip, "127.0.0.1");
    peers[1].port = GOOD_PEER_PORT;

    failure_add_peer(&failure_table, peers[0]);
    failure_add_peer(&failure_table, peers[1]);

    DownloadTask task;
    memset(&task, 0, sizeof(task));

    task.socket_fd = -1;
    task.pieces_dir = TEST_PIECES_DIR;
    task.piece_id = 0;
    task.piece = &piece;
    task.result = -1;

    RecoveryTransferConfig config = {
        .timeout_ms = 1000,
        .max_attempts = 2
    };

    printf("Peer 1 is serving corrupted data.\n");
    printf("Peer 2 is serving correct data.\n");

    int result = recovery_download_piece(
        &task,
        &failure_table,
        &fairness_table,
        peers,
        2,
        config);

    if (result != 0 || task.result != 0) {
        fprintf(stderr, "Corruption recovery failed.\n");
        return 1;
    }

    char piece_path[512];

    snprintf(
        piece_path,
        sizeof(piece_path),
        "%s/piece_%u",
        TEST_PIECES_DIR,
        piece.info.piece_id);

    if (verify_sha256(
            piece_path,
            0,
            TEST_SIZE,
            expected_hash) != 1) {

        fprintf(stderr,
                "Final recovered piece SHA-256 verification failed.\n");
        return 1;
    }

    if (piece.info.status != PIECE_AVAILABLE) {
        fprintf(stderr,
                "Recovered piece is not marked available.\n");
        return 1;
    }

    PeerFailureState *failed_peer =
        failure_find_peer(&failure_table, 1);

    if (failed_peer == NULL ||
        failed_peer->status == PEER_AVAILABLE) {

        fprintf(stderr,
                "Corrupting peer was not marked unavailable.\n");
        return 1;
    }

    close(bad_server);
    close(good_server);

    pthread_join(bad_thread, NULL);
    pthread_join(good_thread, NULL);

    close(bad_fd);
    close(good_fd);

    pthread_mutex_destroy(&piece.mutex);

    unlink(GOOD_FILE);
    unlink(BAD_FILE);
    unlink(piece_path);
    rmdir(TEST_PIECES_DIR);

    printf("Corrupted-piece recovery test passed.\n");
    printf("Peer 1 sent corrupted data.\n");
    printf("SHA-256 corruption detected.\n");
    printf("Piece recovered from Peer 2.\n");
    printf("Final SHA-256 verification passed.\n");

    return 0;
}
