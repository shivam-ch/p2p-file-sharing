#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>

#include "../src/common/types.h"
#include "../src/network/peer.h"
#include "../src/network/discovery.h"
#include "../src/storage/sha256.h"
#include "../src/storage/file_splitter.h"
#include "../src/storage/availability.h"
#include "../src/transfer/transfer.h"

#define TEST_PIECES 3
#define PIECE_SIZE 100000

#define PEER1_PORT 19201
#define PEER2_PORT 19202
#define PEER3_PORT 19203

#define TEST_FILE "/tmp/p2p_dropout_source.bin"
#define SOURCE_PIECES_DIR "/tmp/p2p_dropout_source_pieces"
#define DOWNLOAD_PIECES_DIR "/tmp/p2p_dropout_download_pieces"
#define RECONSTRUCTED_FILE "/tmp/p2p_dropout_reconstructed.bin"


typedef struct
{
    int server_fd;
    int file_fd;
} NormalServerContext;


typedef struct
{
    int server_fd;
    int file_fd;
} DropoutServerContext;


typedef struct
{
    int client_fd;
    int file_fd;
} UploadConnectionContext;


/*
 * Normal server:
 * accepts a fixed number of connections and gives
 * each connection its own upload_worker thread.
 */
static void *normal_server_thread(void *arg)
{
    NormalServerContext *ctx = arg;

    if (ctx == NULL) {
        return NULL;
    }

    UploadConnectionContext connections[2];
    pthread_t workers[2];

    memset(connections, 0, sizeof(connections));
    memset(workers, 0, sizeof(workers));

    /*
     * Peer 2 needs to serve two pieces:
     * Piece 0 and Piece 2.
     */
    for (int i = 0; i < 2; i++) {

        int client_fd = accept_peer(ctx->server_fd);

        if (client_fd < 0) {
            return NULL;
        }

        connections[i].client_fd = client_fd;
        connections[i].file_fd = ctx->file_fd;

        if (pthread_create(
                &workers[i],
                NULL,
                (void *(*)(void *))upload_worker,
                &(UploadTask){
                    .socket_fd = client_fd,
                    .file_fd = ctx->file_fd
                }) != 0) {

            close(client_fd);
            return NULL;
        }
    }

    for (int i = 0; i < 2; i++) {
        pthread_join(workers[i], NULL);
        close(connections[i].client_fd);
    }

    return NULL;
}


/*
 * Peer 3 only needs to serve Piece 1.
 */
static void *single_server_thread(void *arg)
{
    NormalServerContext *ctx = arg;

    if (ctx == NULL) {
        return NULL;
    }

    int client_fd = accept_peer(ctx->server_fd);

    if (client_fd < 0) {
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
 * Peer 1 intentionally sends only the first block
 * and then closes the connection.
 *
 * This creates a genuine MID-TRANSFER dropout.
 */
static void *dropout_server_thread(void *arg)
{
    DropoutServerContext *ctx = arg;

    if (ctx == NULL) {
        return NULL;
    }

    int client_fd = accept_peer(ctx->server_fd);

    if (client_fd < 0) {
        return NULL;
    }

    PieceRequest request;

    if (receive_piece_request(client_fd, &request) != 0) {
        close(client_fd);
        return NULL;
    }

    unsigned char data[MAX_PIECE_BLOCK_DATA];

    uint32_t first_size =
        request.size < MAX_PIECE_BLOCK_DATA
            ? (uint32_t)request.size
            : MAX_PIECE_BLOCK_DATA;

    ssize_t bytes_read = pread(
        ctx->file_fd,
        data,
        first_size,
        (off_t)request.offset
    );

    if (bytes_read != (ssize_t)first_size) {
        close(client_fd);
        return NULL;
    }

    PieceBlock block;

    block.piece_id = request.piece_id;
    block.offset = request.offset;
    block.data_size = first_size;

    if (send_piece_block(
            client_fd,
            &block,
            data
        ) == 0) {

        printf(
            "Peer 1 sent the first block of Piece %u "
            "and dropped connection.\n",
            request.piece_id
        );
    }

    /*
     * Close before sending the remaining blocks.
     */
    close(client_fd);

    return NULL;
}


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

    for (int piece = 0; piece < TEST_PIECES; piece++) {

        for (size_t i = 0; i < sizeof(buffer); i++) {
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


static void cleanup(void)
{
    for (int i = 0; i < TEST_PIECES; i++) {

        char path[512];

        snprintf(
            path,
            sizeof(path),
            "%s/piece_%d",
            SOURCE_PIECES_DIR,
            i
        );

        unlink(path);

        snprintf(
            path,
            sizeof(path),
            "%s/piece_%d",
            DOWNLOAD_PIECES_DIR,
            i
        );

        unlink(path);
    }

    rmdir(SOURCE_PIECES_DIR);
    rmdir(DOWNLOAD_PIECES_DIR);

    unlink(TEST_FILE);
    unlink(RECONSTRUCTED_FILE);
}


typedef struct
{
    AvailabilityTable *availability;
    PeerTable *peers;
    uint32_t piece_id;
    TransferPiece *piece;
    const char *pieces_dir;
    int result;
} DownloadContext;


static void *download_thread(void *arg)
{
    DownloadContext *ctx = arg;

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
            "Piece %u download FAILED.\n",
            ctx->piece_id
        );
    }

    return NULL;
}


int main(void)
{
    printf(
        "Starting 3-peer mid-transfer dropout recovery test...\n"
    );

    cleanup();

    if (create_source_file() != 0) {
        return 1;
    }

    system("mkdir -p " SOURCE_PIECES_DIR);
    system("mkdir -p " DOWNLOAD_PIECES_DIR);

    /*
     * Split source into exactly 3 pieces.
     */
    PieceInfo *pieces = NULL;
    size_t piece_count = 0;

    if (split_file(
            TEST_FILE,
            SOURCE_PIECES_DIR,
            PIECE_SIZE,
            &pieces,
            &piece_count
        ) != 0) {

        fprintf(stderr, "split_file failed\n");
        cleanup();
        return 1;
    }

    if (piece_count != TEST_PIECES) {
        fprintf(
            stderr,
            "Expected %d pieces, got %zu\n",
            TEST_PIECES,
            piece_count
        );

        free(pieces);
        cleanup();
        return 1;
    }

    printf("Source file split into 3 pieces.\n");

    /*
     * Start the three peers.
     */
    int peer1_fd = start_listener(PEER1_PORT);
    int peer2_fd = start_listener(PEER2_PORT);
    int peer3_fd = start_listener(PEER3_PORT);

    if (peer1_fd < 0 ||
        peer2_fd < 0 ||
        peer3_fd < 0) {

        fprintf(stderr, "Failed to start one or more peers\n");

        if (peer1_fd >= 0) close(peer1_fd);
        if (peer2_fd >= 0) close(peer2_fd);
        if (peer3_fd >= 0) close(peer3_fd);

        free(pieces);
        cleanup();

        return 1;
    }

    printf(
        "Peer 1 listening on port %d\n",
        PEER1_PORT
    );

    printf(
        "Peer 2 listening on port %d\n",
        PEER2_PORT
    );

    printf(
        "Peer 3 listening on port %d\n",
        PEER3_PORT
    );


    /*
     * Open source file descriptors.
     */
    int peer1_file = open(TEST_FILE, O_RDONLY);
    int peer2_file = open(TEST_FILE, O_RDONLY);
    int peer3_file = open(TEST_FILE, O_RDONLY);

    if (peer1_file < 0 ||
        peer2_file < 0 ||
        peer3_file < 0) {

        fprintf(stderr, "Failed to open source file\n");

        if (peer1_file >= 0) close(peer1_file);
        if (peer2_file >= 0) close(peer2_file);
        if (peer3_file >= 0) close(peer3_file);

        close(peer1_fd);
        close(peer2_fd);
        close(peer3_fd);

        free(pieces);
        cleanup();

        return 1;
    }


    /*
     * Start server threads.
     */
    DropoutServerContext peer1_context;

    peer1_context.server_fd = peer1_fd;
    peer1_context.file_fd = peer1_file;

    NormalServerContext peer2_context;

    peer2_context.server_fd = peer2_fd;
    peer2_context.file_fd = peer2_file;

    NormalServerContext peer3_context;

    peer3_context.server_fd = peer3_fd;
    peer3_context.file_fd = peer3_file;

    pthread_t peer1_thread;
    pthread_t peer2_thread;
    pthread_t peer3_thread;

    pthread_create(
        &peer1_thread,
        NULL,
        dropout_server_thread,
        &peer1_context
    );

    pthread_create(
        &peer2_thread,
        NULL,
        normal_server_thread,
        &peer2_context
    );

    pthread_create(
        &peer3_thread,
        NULL,
        single_server_thread,
        &peer3_context
    );


    /*
     * Build peer table.
     */
    PeerTable peers_table;

    peer_table_init(&peers_table);

    PeerInfo peer;

    memset(&peer, 0, sizeof(peer));
    peer.peer_id = 1;
    strcpy(peer.ip, "127.0.0.1");
    peer.port = PEER1_PORT;
    peer_table_add(&peers_table, peer);

    memset(&peer, 0, sizeof(peer));
    peer.peer_id = 2;
    strcpy(peer.ip, "127.0.0.1");
    peer.port = PEER2_PORT;
    peer_table_add(&peers_table, peer);

    memset(&peer, 0, sizeof(peer));
    peer.peer_id = 3;
    strcpy(peer.ip, "127.0.0.1");
    peer.port = PEER3_PORT;
    peer_table_add(&peers_table, peer);


    /*
     * Build piece availability.
     *
     * Piece 0:
     *     Peer 1 first -> intentionally drops.
     *     Peer 2 second -> recovery.
     *
     * Piece 1:
     *     Peer 3.
     *
     * Piece 2:
     *     Peer 2.
     */
    AvailabilityTable availability;

    availability_init(&availability);

    for (uint32_t i = 0; i < TEST_PIECES; i++) {
        availability_add_piece(&availability, i);
    }

    availability_add_peer(&availability, 0, 1);
    availability_add_peer(&availability, 0, 2);

    availability_add_peer(&availability, 1, 3);

    availability_add_peer(&availability, 2, 2);

    printf("\nPiece availability:\n");
    printf("  Piece 0 -> Peer 1, Peer 2\n");
    printf("  Piece 1 -> Peer 3\n");
    printf("  Piece 2 -> Peer 2\n");

    printf("\nStarting concurrent downloads...\n\n");


    /*
     * Create TransferPiece objects.
     *
     * The metadata from split_file contains the expected
     * SHA-256 hashes. The downloaded pieces start as missing.
     */
    TransferPiece transfer_pieces[TEST_PIECES];

    memset(
        transfer_pieces,
        0,
        sizeof(transfer_pieces)
    );

    for (int i = 0; i < TEST_PIECES; i++) {

        transfer_pieces[i].info = pieces[i];
        transfer_pieces[i].info.status = PIECE_MISSING;

        transfer_pieces[i].state = TRANSFER_MISSING;

        if (pthread_mutex_init(
                &transfer_pieces[i].mutex,
                NULL
            ) != 0) {

            fprintf(stderr, "pthread_mutex_init failed\n");

            return 1;
        }
    }


    /*
     * Start all three piece downloads concurrently.
     */
    DownloadContext download_contexts[TEST_PIECES];
    pthread_t download_threads[TEST_PIECES];

    for (int i = 0; i < TEST_PIECES; i++) {

        download_contexts[i].availability = &availability;
        download_contexts[i].peers = &peers_table;
        download_contexts[i].piece_id = i;
        download_contexts[i].piece = &transfer_pieces[i];
        download_contexts[i].pieces_dir =
            DOWNLOAD_PIECES_DIR;
        download_contexts[i].result = -1;

        pthread_create(
            &download_threads[i],
            NULL,
            download_thread,
            &download_contexts[i]
        );
    }

    /*
     * Wait for all downloads.
     */
    int all_downloads_ok = 1;

    for (int i = 0; i < TEST_PIECES; i++) {

        pthread_join(
            download_threads[i],
            NULL
        );

        if (download_contexts[i].result != 0) {
            all_downloads_ok = 0;
        }

        /*
         * TransferPiece now contains the downloaded piece.
         */
        if (download_contexts[i].result == 0) {
            pieces[i].status = PIECE_AVAILABLE;
        }
    }

    if (!all_downloads_ok) {

        fprintf(
            stderr,
            "\nOne or more pieces failed to download.\n"
        );

        pthread_cancel(peer1_thread);
        pthread_cancel(peer2_thread);
        pthread_cancel(peer3_thread);

        return 1;
    }

    printf(
        "\nAll three pieces downloaded successfully.\n"
    );


    /*
     * Reconstruct the COMPLETE file from all 3 pieces.
     */
    if (reconstruct_file(
            RECONSTRUCTED_FILE,
            DOWNLOAD_PIECES_DIR,
            pieces,
            TEST_PIECES
        ) != 0) {

        fprintf(
            stderr,
            "File reconstruction failed.\n"
        );

        return 1;
    }

    printf("File reconstructed successfully.\n");


    /*
     * Verify the COMPLETE reconstructed file.
     */
    unsigned char original_hash[SHA256_HASH_SIZE];
    unsigned char reconstructed_hash[SHA256_HASH_SIZE];

    if (calculate_sha256(
            TEST_FILE,
            0,
            TEST_PIECES * PIECE_SIZE,
            original_hash
        ) != 0) {

        fprintf(
            stderr,
            "Failed to calculate original SHA-256.\n"
        );

        return 1;
    }

    if (calculate_sha256(
            RECONSTRUCTED_FILE,
            0,
            TEST_PIECES * PIECE_SIZE,
            reconstructed_hash
        ) != 0) {

        fprintf(
            stderr,
            "Failed to calculate reconstructed SHA-256.\n"
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
            "Final SHA-256 verification FAILED.\n"
        );

        return 1;
    }


    /*
     * Wait for peer server threads to finish.
     */
    pthread_join(peer1_thread, NULL);
    pthread_join(peer2_thread, NULL);
    pthread_join(peer3_thread, NULL);

    close(peer1_file);
    close(peer2_file);
    close(peer3_file);

    close(peer1_fd);
    close(peer2_fd);
    close(peer3_fd);

    for (int i = 0; i < TEST_PIECES; i++) {
        pthread_mutex_destroy(
            &transfer_pieces[i].mutex
        );
    }

    availability_free(&availability);
    free(pieces);

    cleanup();

    printf("\n");
    printf(
        "3-peer mid-transfer dropout recovery test passed.\n"
    );
    printf(
        "Peer 1 dropped during Piece 0 transfer.\n"
    );
    printf(
        "Piece 0 was recovered from Peer 2.\n"
    );
    printf(
        "All 3 pieces were downloaded concurrently.\n"
    );
    printf(
        "Full file reconstruction passed.\n"
    );
    printf(
        "Final SHA-256 verification passed.\n"
    );

    return 0;
}
