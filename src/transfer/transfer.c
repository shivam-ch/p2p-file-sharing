#define _POSIX_C_SOURCE 200809L
#include "transfer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../common/protocol.h"
#include "../storage/piece.h"
#include "../storage/file_io.h"
#include "../storage/sha256.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>

static uint64_t host_to_network_u64(uint64_t value)
{
    uint32_t high = (uint32_t)(value >> 32);
    uint32_t low = (uint32_t)(value & 0xFFFFFFFFULL);

    uint64_t network_high = (uint64_t)htonl(high);
    uint64_t network_low = (uint64_t)htonl(low);

    return (network_low << 32) | network_high;
}

static uint64_t network_to_host_u64(uint64_t value)
{
    uint32_t high = (uint32_t)(value & 0xFFFFFFFFULL);
    uint32_t low = (uint32_t)(value >> 32);

    return ((uint64_t)ntohl(high) << 32) |
           ntohl(low);
}

int download_piece(DownloadTask *task)
{
    PieceRequest request;
    unsigned char data[MAX_PIECE_BLOCK_DATA];
    char piece_path[1024];
    uint64_t received = 0;

    if (task == NULL ||
        task->piece == NULL ||
        task->socket_fd < 0 ||
        task->pieces_dir == NULL) {
        return -1;
    }

    if (task->piece->info.size == 0) {
        return -1;
    }

    /*
     * Create the pieces directory if it does not exist.
     */
    if (mkdir(task->pieces_dir, 0755) != 0) {
        struct stat st;

        if (stat(task->pieces_dir, &st) != 0 ||
            !S_ISDIR(st.st_mode)) {
            return -1;
        }
    }

    snprintf(
        piece_path,
        sizeof(piece_path),
        "%s/piece_%u",
        task->pieces_dir,
        task->piece->info.piece_id
    );

    /*
     * Remove an old partial copy before starting again.
     */
    remove(piece_path);

    request.piece_id = task->piece->info.piece_id;
    request.offset = task->piece->info.offset;
    request.size = task->piece->info.size;

    if (send_piece_request(
            task->socket_fd,
            &request
        ) != 0) {
        return -1;
    }

    while (received < request.size) {
        PieceBlock block;
        uint32_t expected_size;
        uint64_t remaining;

        remaining = request.size - received;

        expected_size =
            remaining > MAX_PIECE_BLOCK_DATA
                ? MAX_PIECE_BLOCK_DATA
                : (uint32_t)remaining;

        if (receive_piece_block(
                task->socket_fd,
                &block,
                data,
                sizeof(data)
            ) != 0) {
            remove(piece_path);
            return -1;
        }

        /*
         * Make sure the block belongs to the requested piece.
         */
        if (block.piece_id != request.piece_id) {
            remove(piece_path);
            return -1;
        }

        /*
         * Blocks must arrive in order for this simple
         * piece reconstruction implementation.
         */
        if (block.offset != request.offset + received) {
            remove(piece_path);
            return -1;
        }

        if (block.data_size == 0 ||
            block.data_size > expected_size) {
            remove(piece_path);
            return -1;
        }

        /*
         * Piece files are stored from offset 0.
         * The network block offset is the global file offset,
         * so convert it to an offset relative to this piece.
         */
        if (write_piece_block(
                piece_path,
                received,
                block.data_size,
                data
            ) != 0) {
            remove(piece_path);
            return -1;
        }

        received += block.data_size;
    }

    /*
     * Verify the complete reconstructed piece.
     */
    if (verify_sha256(
            piece_path,
            0,
            request.size,
            task->piece->info.hash
        ) != 1) {
        remove(piece_path);
        return -1;
    }

    task->piece->info.status = PIECE_AVAILABLE;

    return 0;
}

static void *download_worker_thread(void *arg)
{
    DownloadTask *task = arg;

    if (task == NULL || task->piece == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&task->piece->mutex);

    if (task->piece->state != TRANSFER_MISSING) {
        task->result = -1;
        pthread_mutex_unlock(&task->piece->mutex);
        return NULL;
    }

    task->piece->state = TRANSFER_DOWNLOADING;

    pthread_mutex_unlock(&task->piece->mutex);

    task->result = download_piece(task);

    pthread_mutex_lock(&task->piece->mutex);

    if (task->result == 0) {
        task->piece->state = TRANSFER_COMPLETED;
    } else {
        task->piece->state = TRANSFER_FAILED;
    }

    pthread_mutex_unlock(&task->piece->mutex);

    return NULL;
}

void *upload_worker(void *arg)
{
    UploadTask *task = arg;
    unsigned char data[MAX_PIECE_BLOCK_DATA];

    if (task == NULL ||
        task->socket_fd < 0 ||
        task->file_fd < 0) {
        return NULL;
    }

    while (1) {
        PieceRequest request;

        if (receive_piece_request(
                task->socket_fd,
                &request
            ) != 0) {
            break;
        }

        if (request.size == 0) {
            break;
        }

        uint64_t sent = 0;

        while (sent < request.size) {
            uint64_t remaining = request.size - sent;

            uint32_t block_size =
                remaining > MAX_PIECE_BLOCK_DATA
                    ? MAX_PIECE_BLOCK_DATA
                    : (uint32_t)remaining;

            ssize_t bytes_read = pread(
                task->file_fd,
                data,
                block_size,
                (off_t)(request.offset + sent)
            );

            if (bytes_read < 0 ||
                (uint32_t)bytes_read != block_size) {
                return NULL;
            }

            PieceBlock block;

            block.piece_id = request.piece_id;
            block.offset = request.offset + sent;
            block.data_size = block_size;

            if (send_piece_block(
                    task->socket_fd,
                    &block,
                    data
                ) != 0) {
                return NULL;
            }

            sent += block_size;
        }
    }

    return NULL;
}

int start_downloads(
    DownloadTask *tasks,
    size_t task_count,
    size_t thread_count
)
{
    if (tasks == NULL ||
        task_count == 0 ||
        thread_count == 0) {
        return -1;
    }

    if (thread_count > task_count) {
        thread_count = task_count;
    }

    pthread_t *threads = malloc(
        thread_count * sizeof(pthread_t)
    );

    if (threads == NULL) {
        return -1;
    }

    size_t started = 0;
    int result = 0;

    while (started < task_count) {
        size_t batch = 0;

        while (batch < thread_count &&
               started + batch < task_count) {

            tasks[started + batch].result = -1;

            if (pthread_create(
                    &threads[batch],
                    NULL,
                    download_worker_thread,
                    &tasks[started + batch]
                ) != 0) {
                break;
            }

            batch++;
        }

        for (size_t i = 0; i < batch; i++) {
            pthread_join(threads[i], NULL);
        }

        if (batch == 0) {
            result = -1;
            break;
        }

        for (size_t i = 0; i < batch; i++) {
            if (tasks[started + i].result != 0) {
                result = -1;
            }
        }

        started += batch;
    }

    free(threads);

    return result;
}

int serialize_piece_request(
    const PieceRequest *request,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
)
{
    uint32_t network_piece_id;
    uint64_t network_offset;
    uint64_t network_size;

    if (request == NULL ||
        buffer == NULL ||
        output_size == NULL) {
        return -1;
    }

    if (buffer_size < 20) {
        return -1;
    }

    network_piece_id = htonl(request->piece_id);
    network_offset = host_to_network_u64(request->offset);
    network_size = host_to_network_u64(request->size);

    memcpy(
        buffer,
        &network_piece_id,
        sizeof(network_piece_id)
    );

    memcpy(
        buffer + 4,
        &network_offset,
        sizeof(network_offset)
    );

    memcpy(
        buffer + 12,
        &network_size,
        sizeof(network_size)
    );

    *output_size = 20;

    return 0;
}

int deserialize_piece_request(
    const unsigned char *buffer,
    uint32_t buffer_size,
    PieceRequest *request
)
{
    uint32_t network_piece_id;
    uint64_t network_offset;
    uint64_t network_size;

    if (buffer == NULL ||
        request == NULL) {
        return -1;
    }

    if (buffer_size != 20) {
        return -1;
    }

    memcpy(
        &network_piece_id,
        buffer,
        sizeof(network_piece_id)
    );

    memcpy(
        &network_offset,
        buffer + 4,
        sizeof(network_offset)
    );

    memcpy(
        &network_size,
        buffer + 12,
        sizeof(network_size)
    );

    request->piece_id = ntohl(network_piece_id);
    request->offset = network_to_host_u64(network_offset);
    request->size = network_to_host_u64(network_size);

    return 0;
}

int send_piece_request(
    int socket_fd,
    const PieceRequest *request
)
{
    unsigned char buffer[20];
    uint32_t payload_size;

    if (request == NULL) {
        return -1;
    }

    if (serialize_piece_request(
            request,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    return send_message(
        socket_fd,
        MSG_PIECE_REQUEST,
        buffer,
        payload_size
    );
}

int receive_piece_request(
    int socket_fd,
    PieceRequest *request
)
{
    unsigned char buffer[20];
    MessageType message_type;
    uint32_t payload_size;

    if (request == NULL) {
        return -1;
    }

    if (receive_message(
            socket_fd,
            &message_type,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    if (message_type != MSG_PIECE_REQUEST) {
        return -1;
    }

    return deserialize_piece_request(
        buffer,
        payload_size,
        request
    );
}
int serialize_piece_block(
    const PieceBlock *block,
    const unsigned char *data,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
)
{
    uint32_t network_piece_id;
    uint64_t network_offset;
    uint32_t network_data_size;

    if (block == NULL ||
        data == NULL ||
        buffer == NULL ||
        output_size == NULL) {
        return -1;
    }

    if (block->data_size > MAX_PIECE_BLOCK_DATA) {
        return -1;
    }

    if (buffer_size < PIECE_BLOCK_METADATA_SIZE + block->data_size) {
        return -1;
    }

    network_piece_id = htonl(block->piece_id);
    network_offset = host_to_network_u64(block->offset);
    network_data_size = htonl(block->data_size);

    memcpy(buffer, &network_piece_id, 4);
    memcpy(buffer + 4, &network_offset, 8);
    memcpy(buffer + 12, &network_data_size, 4);

    memcpy(
        buffer + PIECE_BLOCK_METADATA_SIZE,
        data,
        block->data_size
    );

    *output_size = PIECE_BLOCK_METADATA_SIZE + block->data_size;

    return 0;
}
int deserialize_piece_block(
    const unsigned char *buffer,
    uint32_t buffer_size,
    PieceBlock *block,
    unsigned char *data,
    uint32_t data_capacity
)
{
    uint32_t network_piece_id;
    uint64_t network_offset;
    uint32_t network_data_size;

    if (buffer == NULL ||
        block == NULL ||
        data == NULL) {
        return -1;
    }

    if (buffer_size < PIECE_BLOCK_METADATA_SIZE) {
        return -1;
    }

    memcpy(&network_piece_id, buffer, 4);
    memcpy(&network_offset, buffer + 4, 8);
    memcpy(&network_data_size, buffer + 12, 4);

    block->piece_id = ntohl(network_piece_id);
    block->offset = network_to_host_u64(network_offset);
    block->data_size = ntohl(network_data_size);

    if (block->data_size > MAX_PIECE_BLOCK_DATA) {
        return -1;
    }

    if (buffer_size !=
        PIECE_BLOCK_METADATA_SIZE + block->data_size) {
        return -1;
    }

    if (block->data_size > data_capacity) {
        return -1;
    }

    memcpy(
        data,
        buffer + PIECE_BLOCK_METADATA_SIZE,
        block->data_size
    );

    return 0;
}
int send_piece_block(
    int socket_fd,
    const PieceBlock *block,
    const unsigned char *data
)
{
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    uint32_t payload_size;

    if (block == NULL || data == NULL) {
        return -1;
    }

    if (serialize_piece_block(
            block,
            data,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    return send_message(
        socket_fd,
        MSG_PIECE_RESPONSE,
        buffer,
        payload_size
    );
}
int receive_piece_block(
    int socket_fd,
    PieceBlock *block,
    unsigned char *data,
    uint32_t data_capacity
)
{
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    MessageType message_type;
    uint32_t payload_size;

    if (block == NULL || data == NULL) {
        return -1;
    }

    if (receive_message(
            socket_fd,
            &message_type,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    if (message_type != MSG_PIECE_RESPONSE) {
        return -1;
    }

    return deserialize_piece_block(
        buffer,
        payload_size,
        block,
        data,
        data_capacity
    );
}
int upload_piece_block(
    int socket_fd,
    const char *file_path,
    const PieceRequest *request
)
{
    unsigned char data[MAX_PIECE_BLOCK_DATA];
    PieceBlock block;

    if (file_path == NULL || request == NULL) {
        return -1;
    }

    if (request->size == 0 ||
        request->size > MAX_PIECE_BLOCK_DATA) {
        return -1;
    }

    block.piece_id = request->piece_id;
    block.offset = request->offset;
    block.data_size = (uint32_t)request->size;

    if (read_piece_block(
            file_path,
            request->offset,
            block.data_size,
            data
        ) != 0) {
        return -1;
    }

    return send_piece_block(
        socket_fd,
        &block,
        data
    );
}