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

#define FAILED_PEER_PORT 19093
#define WORKING_PEER_PORT 19094

#define TEST_FILE "/tmp/p2p_recovery_source.bin"
#define TEST_PIECES_DIR "/tmp/p2p_recovery_pieces"

#define TEST_SIZE 100000

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

static int create_source_file(void)
{
    unsigned char buffer[TEST_SIZE];

    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = (unsigned char)((i * 37) % 251);
    }

    int fd = open(TEST_FILE, O_CREAT | O_TRUNC | O_WRONLY, 0644);

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
    printf("Starting recovery transfer integration test...\n");

    if (create_source_file() != 0) {
        return 1;
    }

    system("rm -rf " TEST_PIECES_DIR);

    if (mkdir(TEST_PIECES_DIR, 0755) != 0) {
        perror("mkdir");
        unlink(TEST_FILE);
        return 1;
    }

    /*
     * Only Peer 2 is actually running.
     * Peer 1 intentionally has no listener, so the
     * recovery layer must fail over to Peer 2.
     */
    int server_fd = start_listener(WORKING_PEER_PORT);

    if (server_fd < 0) {
        fprintf(stderr, "start_listener failed\n");
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    int source_fd = open(TEST_FILE, O_RDONLY);

    if (source_fd < 0) {
        perror("open source");
        close(server_fd);
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    unsigned char expected_hash[SHA256_HASH_SIZE];

    if (calculate_sha256(
            TEST_FILE,
            0,
            TEST_SIZE,
            expected_hash) != 0) {

        fprintf(stderr, "calculate_sha256 failed\n");

        close(source_fd);
        close(server_fd);
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    TransferPiece piece;
    memset(&piece, 0, sizeof(piece));

    piece.info.piece_id = 0;
    piece.info.offset = 0;
    piece.info.size = TEST_SIZE;
    memcpy(piece.info.hash, expected_hash, SHA256_HASH_SIZE);
    piece.info.status = PIECE_MISSING;
    piece.state = TRANSFER_MISSING;

    if (pthread_mutex_init(&piece.mutex, NULL) != 0) {
        fprintf(stderr, "pthread_mutex_init failed\n");

        close(source_fd);
        close(server_fd);
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
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
    peers[0].port = FAILED_PEER_PORT;

    peers[1].peer_id = 2;
    strcpy(peers[1].ip, "127.0.0.1");
    peers[1].port = WORKING_PEER_PORT;

    failure_add_peer(&failure_table, peers[0]);
    failure_add_peer(&failure_table, peers[1]);

    ServerContext server_ctx;
    server_ctx.server_fd = server_fd;
    server_ctx.file_fd = source_fd;

    pthread_t server_thread_id;

    if (pthread_create(
            &server_thread_id,
            NULL,
            server_thread,
            &server_ctx) != 0) {

        fprintf(stderr, "pthread_create failed\n");

        pthread_mutex_destroy(&piece.mutex);
        close(source_fd);
        close(server_fd);
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    DownloadTask task;
    memset(&task, 0, sizeof(task));

    task.socket_fd = -1;
    task.pieces_dir = TEST_PIECES_DIR;
    task.piece_id = 0;
    task.piece = &piece;
    task.result = -1;

    RecoveryTransferConfig config;
    config.timeout_ms = 1000;
    config.max_attempts = 2;

    printf("Peer 1 is intentionally unavailable.\n");
    printf("Peer 2 is serving the requested piece.\n");

    int result = recovery_download_piece(
        &task,
        &failure_table,
        &fairness_table,
        peers,
        2,
        config);

    if (result != 0 || task.result != 0) {
        fprintf(stderr, "Recovery transfer failed.\n");

        pthread_join(server_thread_id, NULL);
        pthread_mutex_destroy(&piece.mutex);
        close(source_fd);
        close(server_fd);
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
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

        fprintf(stderr, "Recovered piece SHA-256 verification failed.\n");

        pthread_join(server_thread_id, NULL);
        pthread_mutex_destroy(&piece.mutex);
        close(source_fd);
        close(server_fd);
        unlink(TEST_FILE);
        unlink(piece_path);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    if (piece.info.status != PIECE_AVAILABLE) {
        fprintf(stderr, "Recovered piece is not marked available.\n");

        pthread_join(server_thread_id, NULL);
        pthread_mutex_destroy(&piece.mutex);
        close(source_fd);
        close(server_fd);
        unlink(TEST_FILE);
        unlink(piece_path);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    PeerFailureState *failed_peer =
        failure_find_peer(&failure_table, 1);

    if (failed_peer == NULL ||
        failed_peer->status == PEER_AVAILABLE) {

        fprintf(stderr, "Peer 1 was not marked unavailable.\n");

        pthread_join(server_thread_id, NULL);
        pthread_mutex_destroy(&piece.mutex);
        close(source_fd);
        close(server_fd);
        unlink(TEST_FILE);
        unlink(piece_path);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    close(server_fd);
    pthread_join(server_thread_id, NULL);

    pthread_mutex_destroy(&piece.mutex);

    close(source_fd);

    unlink(TEST_FILE);
    unlink(piece_path);
    rmdir(TEST_PIECES_DIR);

    printf("Recovery transfer test passed.\n");
    printf("Peer 1 failure detected.\n");
    printf("Piece recovered from Peer 2.\n");
    printf("SHA-256 verification passed.\n");

    return 0;
}
