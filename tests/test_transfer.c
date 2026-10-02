#include "../src/transfer/transfer.h"
#include "../src/storage/sha256.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/stat.h>

#define TEST_PIECES 4
#define PIECE_SIZE 1024

typedef struct
{
    int socket_fd;
    int file_fd;
} UploadContext;

static int create_source_file(
    const char *path
)
{
    unsigned char data[TEST_PIECES * PIECE_SIZE];

    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)(i % 251);
    }

    int fd = open(
        path,
        O_CREAT | O_WRONLY | O_TRUNC,
        0600
    );

    if (fd < 0) {
        return -1;
    }

    if (write(fd, data, sizeof(data)) != (ssize_t)sizeof(data)) {
        close(fd);
        return -1;
    }

    close(fd);
    return 0;
}

static int verify_piece_file(
    const char *pieces_dir,
    uint32_t piece_id
)
{
    char path[256];

    snprintf(
        path,
        sizeof(path),
        "%s/piece_%u",
        pieces_dir,
        piece_id
    );

    int fd = open(path, O_RDONLY);

    if (fd < 0) {
        return -1;
    }

    unsigned char buffer[PIECE_SIZE];

    ssize_t bytes_read = read(
        fd,
        buffer,
        sizeof(buffer)
    );

    close(fd);

    if (bytes_read != PIECE_SIZE) {
        return -1;
    }

    for (size_t i = 0; i < PIECE_SIZE; i++) {
        size_t global_index =
            ((size_t)piece_id * PIECE_SIZE) + i;

        unsigned char expected =
            (unsigned char)(global_index % 251);

        if (buffer[i] != expected) {
            return -1;
        }
    }

    return 0;
}

int main(void)
{
    const char *source_path =
        "/tmp/test_concurrent_transfer_source.bin";

    const char *pieces_dir =
        "/tmp/test_concurrent_transfer_pieces";

    if (create_source_file(source_path) != 0) {
        perror("create_source_file");
        return 1;
    }

    if (mkdir(pieces_dir, 0700) != 0) {
        if (access(pieces_dir, F_OK) != 0) {
            perror("mkdir");
            remove(source_path);
            return 1;
        }
    }

    DownloadTask tasks[TEST_PIECES];
    TransferPiece pieces[TEST_PIECES];

    int client_sockets[TEST_PIECES];
    int server_sockets[TEST_PIECES];
    int source_fds[TEST_PIECES];
    pthread_t upload_threads[TEST_PIECES];

    memset(tasks, 0, sizeof(tasks));
    memset(pieces, 0, sizeof(pieces));

    for (int i = 0; i < TEST_PIECES; i++) {
        int sockets[2];

        if (socketpair(
                AF_UNIX,
                SOCK_STREAM,
                0,
                sockets
            ) != 0) {

            perror("socketpair");

            return 1;
        }

        server_sockets[i] = sockets[0];
        client_sockets[i] = sockets[1];

        source_fds[i] = open(
            source_path,
            O_RDONLY
        );

        if (source_fds[i] < 0) {
            perror("open source");

            return 1;
        }

        pieces[i].info.piece_id = i;
        pieces[i].info.offset =
            (uint64_t)i * PIECE_SIZE;
        pieces[i].info.size = PIECE_SIZE;
        pieces[i].info.status = PIECE_MISSING;

        if (calculate_sha256(
                source_path,
                pieces[i].info.offset,
                pieces[i].info.size,
                pieces[i].info.hash
            ) != 0) {

            printf(
                "SHA-256 calculation failed for piece %d.\n",
                i
            );

            return 1;
        }

        pieces[i].state = TRANSFER_MISSING;

        if (pthread_mutex_init(
                &pieces[i].mutex,
                NULL
            ) != 0) {

            printf(
                "Mutex initialization failed for piece %d.\n",
                i
            );

            return 1;
        }

        UploadTask *upload_task =
            malloc(sizeof(UploadTask));

        if (upload_task == NULL) {
            printf("Memory allocation failed.\n");
            return 1;
        }

        upload_task->socket_fd = server_sockets[i];
        upload_task->file_fd = source_fds[i];

        if (pthread_create(
                &upload_threads[i],
                NULL,
                upload_worker,
                upload_task
            ) != 0) {

            perror("pthread_create");
            free(upload_task);
            return 1;
        }

        tasks[i].socket_fd = client_sockets[i];
        tasks[i].pieces_dir = pieces_dir;
        tasks[i].piece_id = i;
        tasks[i].piece = &pieces[i];
        tasks[i].result = -1;
    }

    printf(
        "Starting %d concurrent download workers...\n",
        TEST_PIECES
    );

    if (start_downloads(
            tasks,
            TEST_PIECES,
            TEST_PIECES
        ) != 0) {

        printf("Concurrent download test failed.\n");

        return 1;
    }

    for (int i = 0; i < TEST_PIECES; i++) {
        close(client_sockets[i]);

        pthread_join(
            upload_threads[i],
            NULL
        );

        close(source_fds[i]);
    }

    for (int i = 0; i < TEST_PIECES; i++) {
        if (pieces[i].info.status != PIECE_AVAILABLE) {
            printf(
                "Piece %d is not PIECE_AVAILABLE.\n",
                i
            );

            return 1;
        }

        if (pieces[i].state != TRANSFER_COMPLETED) {
            printf(
                "Piece %d did not complete.\n",
                i
            );

            return 1;
        }

        if (verify_piece_file(
                pieces_dir,
                i
            ) != 0) {

            printf(
                "Stored piece %d verification failed.\n",
                i
            );

            return 1;
        }

        pthread_mutex_destroy(
            &pieces[i].mutex
        );
    }

    printf(
        "Concurrent download test passed.\n"
    );

    printf(
        "All %d pieces transferred successfully.\n",
        TEST_PIECES
    );

    printf(
        "All pieces passed SHA-256 verification.\n"
    );

    for (int i = 0; i < TEST_PIECES; i++) {
        char path[256];

        snprintf(
            path,
            sizeof(path),
            "%s/piece_%d",
            pieces_dir,
            i
        );

        remove(path);
    }

    rmdir(pieces_dir);
    remove(source_path);

    return 0;
}
