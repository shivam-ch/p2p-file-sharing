#ifndef TRANSFER_H
#define TRANSFER_H
#include <stdint.h>
#include <pthread.h>
#include "../common/types.h"

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
} 
TransferPiece;
typedef struct
 {
    int socket_fd;
    uint32_t piece_id;
    TransferPiece *piece;
} 
DownloadTask;
typedef struct {
    uint32_t piece_id;
    uint64_t offset;
    uint64_t size;
} 
PieceRequest;
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

#endif
