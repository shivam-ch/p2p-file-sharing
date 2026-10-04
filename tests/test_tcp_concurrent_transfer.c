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

#define TEST_PORT 19092
#define TEST_PIECES 4
#define PIECE_SIZE 100000

#define TEST_FILE "/tmp/p2p_tcp_concurrent_source.bin"
#define TEST_PIECES_DIR "/tmp/p2p_tcp_concurrent_pieces"

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
    unsigned char buffer[PIECE_SIZE];

    int fd = open(
        TEST_FILE,
        O_CREAT | O_TRUNC | O_WRONLY,
        0644);

    if (fd < 0) {
        perror("open source file");
        return -1;
    }

    for (int piece = 0; piece < TEST_PIECES; piece++) {
        for (size_t i = 0; i < sizeof(buffer); i++) {
            buffer[i] = (unsigned char)((piece * 31 + i) % 251);
        }

        if (write(fd, buffer, sizeof(buffer)) != (ssize_t)sizeof(buffer)) {
            perror("write source file");
            close(fd);
            return -1;
        }
    }

    close(fd);
    return 0;
}

int main(void)
{
    printf(
        "Starting %d concurrent TCP download workers...\n",
        TEST_PIECES);

    if (create_source_file() != 0) {
        return 1;
    }

    system("rm -rf " TEST_PIECES_DIR);

    if (mkdir(TEST_PIECES_DIR, 0755) != 0) {
        perror("mkdir");
        unlink(TEST_FILE);
        return 1;
    }

    int server_fd = start_listener(TEST_PORT);
    if (server_fd < 0) {
        fprintf(stderr, "start_listener failed\n");
        unlink(TEST_FILE);
        rmdir(TEST_PIECES_DIR);
        return 1;
    }

    DownloadTask tasks[TEST_PIECES];
    TransferPiece pieces[TEST_PIECES];
    pthread_t server_threads[TEST_PIECES];
    ServerContext contexts[TEST_PIECES];
    int client_fds[TEST_PIECES];
    int source_fds[TEST_PIECES];

    memset(tasks, 0, sizeof(tasks));
    memset(pieces, 0, sizeof(pieces));

    for (int i = 0; i < TEST_PIECES; i++) {
        client_fds[i] = connect_to_peer("127.0.0.1", TEST_PORT);

        if (client_fds[i] < 0) {
            fprintf(stderr, "connect_to_peer failed for piece %d\n", i);
            return 1;
        }

        source_fds[i] = open(TEST_FILE, O_RDONLY);

        if (source_fds[i] < 0) {
            perror("open source");
            return 1;
        }

        pieces[i].info.piece_id = (uint32_t)i;
        pieces[i].info.offset = (uint64_t)i * PIECE_SIZE;
        pieces[i].info.size = PIECE_SIZE;
        pieces[i].info.status = PIECE_MISSING;

        if (calculate_sha256(
                TEST_FILE,
                pieces[i].info.offset,
                pieces[i].info.size,
                pieces[i].info.hash) != 0) {
            fprintf(stderr, "SHA-256 calculation failed for piece %d\n", i);
            return 1;
        }

        pieces[i].state = TRANSFER_MISSING;

        if (pthread_mutex_init(&pieces[i].mutex, NULL) != 0) {
            fprintf(stderr, "pthread_mutex_init failed for piece %d\n", i);
            return 1;
        }

        contexts[i].server_fd = server_fd;
        contexts[i].file_fd = source_fds[i];

        if (pthread_create(
                &server_threads[i],
                NULL,
                server_thread,
                &contexts[i]) != 0) {
            fprintf(stderr, "pthread_create failed for piece %d\n", i);
            return 1;
        }

        tasks[i].socket_fd = client_fds[i];
        tasks[i].pieces_dir = TEST_PIECES_DIR;
        tasks[i].piece_id = (uint32_t)i;
        tasks[i].piece = &pieces[i];
        tasks[i].result = -1;
    }

    int result = start_downloads(
        tasks,
        TEST_PIECES,
        TEST_PIECES);

    if (result != 0) {
        fprintf(stderr, "Concurrent TCP transfer failed\n");
        return 1;
    }

    for (int i = 0; i < TEST_PIECES; i++) {
        char piece_path[512];

        snprintf(
            piece_path,
            sizeof(piece_path),
            "%s/piece_%u",
            TEST_PIECES_DIR,
            (unsigned)i);

        if (verify_sha256(
                piece_path,
                0,
                PIECE_SIZE,
                pieces[i].info.hash) != 1) {
            fprintf(
                stderr,
                "SHA-256 verification failed for piece %d\n",
                i);
            return 1;
        }

        if (pieces[i].info.status != PIECE_AVAILABLE) {
            fprintf(
                stderr,
                "Piece %d is not marked available\n",
                i);
            return 1;
        }
    }

    for (int i = 0; i < TEST_PIECES; i++) {
        close(client_fds[i]);
        pthread_join(server_threads[i], NULL);
        close(source_fds[i]);
        pthread_mutex_destroy(&pieces[i].mutex);
    }

    close(server_fd);

    for (int i = 0; i < TEST_PIECES; i++) {
        char piece_path[512];

        snprintf(
            piece_path,
            sizeof(piece_path),
            "%s/piece_%u",
            TEST_PIECES_DIR,
            (unsigned)i);

        unlink(piece_path);
    }

    rmdir(TEST_PIECES_DIR);
    unlink(TEST_FILE);

    printf("Concurrent TCP transfer test passed.\n");
    printf(
        "All %d pieces transferred concurrently.\n",
        TEST_PIECES);
    printf("All pieces passed SHA-256 verification.\n");

    return 0;
}
