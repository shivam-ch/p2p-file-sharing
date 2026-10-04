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

#define TEST_PORT 19091
#define TEST_FILE "/tmp/p2p_tcp_source.bin"
#define TEST_PIECES_DIR "/tmp/p2p_tcp_pieces"

typedef struct {
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
int main(void)
{
    char content[100000];
    for (size_t i = 0; i < sizeof(content); i++) {
        content[i] = (char)("ABCDEFGHIJKLMNOPQRSTUVWXYZ"[i % 26]);
    }

    size_t content_size = sizeof(content);
    /* Create source file */
    int file_fd = open(TEST_FILE, O_CREAT | O_TRUNC | O_RDWR, 0644);
    if (file_fd < 0) {
        perror("open source file");
        return 1;
    }

    if (write(file_fd, content, content_size) != (ssize_t)content_size) {
        perror("write source file");
        close(file_fd);
        return 1;
    }

    /* Calculate expected piece hash using project API */
    unsigned char expected_hash[SHA256_HASH_SIZE];

    if (calculate_sha256(
            TEST_FILE,
            0,
            content_size,
            expected_hash) != 0) {
        fprintf(stderr, "calculate_sha256 failed\n");
        close(file_fd);
        unlink(TEST_FILE);
        return 1;
    }

    /* Start TCP listener */
    int server_fd = start_listener(TEST_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "start_listener failed\n");
        close(file_fd);
        unlink(TEST_FILE);
        return 1;
    }

    ServerContext server_ctx;
    server_ctx.server_fd = server_fd;
    server_ctx.file_fd = file_fd;

    pthread_t server_thread_id;

    if (pthread_create(
            &server_thread_id,
            NULL,
            server_thread,
            &server_ctx) != 0) {
        fprintf(stderr, "pthread_create failed\n");
        close(server_fd);
        close(file_fd);
        unlink(TEST_FILE);
        return 1;
    }

    /* Connect as downloader */
    int client_fd = connect_to_peer("127.0.0.1", TEST_PORT);
    if (client_fd < 0) {
        fprintf(stderr, "connect_to_peer failed\n");
        close(server_fd);
        close(file_fd);
        pthread_join(server_thread_id, NULL);
        unlink(TEST_FILE);
        return 1;
    }

    /* Prepare destination directory */
    system("rm -rf " TEST_PIECES_DIR);
    if (mkdir(TEST_PIECES_DIR, 0755) != 0) {
        perror("mkdir");
        close(client_fd);
        close(server_fd);
        close(file_fd);
        pthread_join(server_thread_id, NULL);
        unlink(TEST_FILE);
        return 1;
    }

    /* Prepare one piece */
    TransferPiece piece;
    memset(&piece, 0, sizeof(piece));

    piece.info.piece_id = 0;
    piece.info.offset = 0;
    piece.info.size = content_size;
    memcpy(piece.info.hash, expected_hash, SHA256_HASH_SIZE);
    piece.info.status = PIECE_MISSING;
    piece.state = TRANSFER_MISSING;

    if (pthread_mutex_init(&piece.mutex, NULL) != 0) {
        fprintf(stderr, "pthread_mutex_init failed\n");
        close(client_fd);
        close(server_fd);
        close(file_fd);
        pthread_join(server_thread_id, NULL);
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    DownloadTask task;
    memset(&task, 0, sizeof(task));

    task.socket_fd = client_fd;
    task.pieces_dir = TEST_PIECES_DIR;
    task.piece_id = 0;
    task.piece = &piece;
    task.result = -1;

    printf("Starting TCP P2P transfer...\n");

    int result = start_downloads(&task, 1, 1);

    if (result != 0 || task.result != 0) {
        fprintf(stderr, "TCP transfer failed\n");

        pthread_mutex_destroy(&piece.mutex);
        close(client_fd);
        close(server_fd);
        close(file_fd);
        pthread_join(server_thread_id, NULL);
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
        0);

    /* Verify received piece */
    if (verify_sha256(
            piece_path,
            0,
            content_size,
            expected_hash) != 1) {
        fprintf(stderr, "Received piece SHA-256 verification failed\n");

        pthread_mutex_destroy(&piece.mutex);
        close(client_fd);
        close(server_fd);
        close(file_fd);
        pthread_join(server_thread_id, NULL);
        unlink(TEST_FILE);
        unlink(piece_path);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    printf("TCP P2P transfer test passed.\n");
    printf("Piece transferred successfully.\n");
    printf("SHA-256 verification passed.\n");

    /*
     * Closing the client causes upload_worker() to finish
     * its receive loop on the server side.
     */
    close(client_fd);

    pthread_join(server_thread_id, NULL);

    pthread_mutex_destroy(&piece.mutex);

    close(server_fd);
    close(file_fd);

    unlink(TEST_FILE);
    unlink(piece_path);
    rmdir(TEST_PIECES_DIR);

    return 0;
}
