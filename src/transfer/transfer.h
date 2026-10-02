#ifndef TRANSFER_H
#define TRANSFER_H

#include <stdint.h>
#include <stddef.h>
#include <pthread.h>

#include "../common/types.h"
#include "../common/protocol.h"

typedef enum
{
    TRANSFER_MISSING = 0,
    TRANSFER_DOWNLOADING,
    TRANSFER_COMPLETED,
    TRANSFER_FAILED
} PieceState;

typedef struct
{
    PieceInfo info;
    PieceState state;
    pthread_mutex_t mutex;
} TransferPiece;

typedef struct
{
    int socket_fd;
    const char *pieces_dir;
    uint32_t piece_id;
    TransferPiece *piece;
    int result;
} DownloadTask;

typedef struct
{
    int socket_fd;
    int file_fd;
} UploadTask;

typedef struct
{
    uint32_t piece_id;
    uint64_t offset;
    uint64_t size;
} PieceRequest;

typedef struct
{
    uint32_t piece_id;
    uint64_t offset;
    uint32_t data_size;
} PieceBlock;

#define PIECE_BLOCK_METADATA_SIZE 16
#define MAX_PIECE_BLOCK_DATA \
    (MAX_PAYLOAD_SIZE - PIECE_BLOCK_METADATA_SIZE)

int download_piece(DownloadTask *task);

void *upload_worker(void *arg);

int start_downloads(
    DownloadTask *tasks,
    size_t task_count,
    size_t thread_count
);

int serialize_piece_request(
    const PieceRequest *request,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
);

int deserialize_piece_request(
    const unsigned char *buffer,
    uint32_t buffer_size,
    PieceRequest *request
);

int send_piece_request(
    int socket_fd,
    const PieceRequest *request
);

int receive_piece_request(
    int socket_fd,
    PieceRequest *request
);

int serialize_piece_block(
    const PieceBlock *block,
    const unsigned char *data,
    unsigned char *buffer,
    uint32_t buffer_size,
    uint32_t *output_size
);

int deserialize_piece_block(
    const unsigned char *buffer,
    uint32_t buffer_size,
    PieceBlock *block,
    unsigned char *data,
    uint32_t data_capacity
);

int send_piece_block(
    int socket_fd,
    const PieceBlock *block,
    const unsigned char *data
);

int receive_piece_block(
    int socket_fd,
    PieceBlock *block,
    unsigned char *data,
    uint32_t data_capacity
);

#endif
