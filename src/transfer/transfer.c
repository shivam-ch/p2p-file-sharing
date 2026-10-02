#include "transfer.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../common/protocol.h"
#include "../storage/piece.h"

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
    unsigned char buffer[MAX_PAYLOAD_SIZE];
    MessageType message_type;
    uint32_t payload_size;

    if (task == NULL ||
        task->piece == NULL ||
        task->socket_fd < 0 ||
        task->pieces_dir == NULL) {
        return -1;
    }

    if (task->piece->info.size == 0 ||
        task->piece->info.size > MAX_PAYLOAD_SIZE) {
        return -1;
    }

    request.piece_id = task->piece->info.piece_id;
    request.offset = task->piece->info.offset;
    request.size = task->piece->info.size;

    if (send_piece_request(
            task->socket_fd,
            &request
        ) != 0) {
        return -1;
    }

    if (receive_message(
            task->socket_fd,
            &message_type,
            buffer,
            sizeof(buffer),
            &payload_size
        ) != 0) {
        return -1;
    }

    if (message_type != MSG_PIECE_RESPONSE ||
        payload_size != request.size) {
        return -1;
    }

    if (piece_store(
            task->pieces_dir,
            &task->piece->info,
            buffer,
            payload_size
        ) != 0) {
        return -1;
    }

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
    unsigned char buffer[MAX_PAYLOAD_SIZE];

    if (task == NULL ||
        task->socket_fd < 0 ||
        task->file_fd < 0) {
        return NULL;
    }

    while (1) {
        PieceRequest request;
        ssize_t bytes_read;

        if (receive_piece_request(
                task->socket_fd,
                &request
            ) != 0) {
            break;
        }

        if (request.size == 0 ||
            request.size > MAX_PAYLOAD_SIZE) {
            break;
        }

        bytes_read = pread(
            task->file_fd,
            buffer,
            request.size,
            (off_t)request.offset
        );

        if (bytes_read < 0 ||
            (uint64_t)bytes_read != request.size) {
            break;
        }

        if (send_message(
                task->socket_fd,
                MSG_PIECE_RESPONSE,
                buffer,
                (uint32_t)bytes_read
            ) != 0) {
            break;
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
