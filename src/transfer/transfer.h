#ifndef TRANSFER_H
#define TRANSFER_H
#include <stdint.h>
#include <pthread.h>
#include "../common/types.h"

typedef enum 
{
    PIECE_MISSING = 0,
    PIECE_DOWNLOADING,
    PIECE_COMPLETED,
    PIECE_FAILED
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

#endif
