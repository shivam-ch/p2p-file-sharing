#include "transfer.h"
#include <stdlib.h>
static void *download_worker(void *arg)
{
    DownloadTask *task = arg;
    if (task == NULL || task->piece == NULL) 
    {
        return NULL;
    }
    pthread_mutex_lock(&task->piece->mutex);
    if (task->piece->state != PIECE_MISSING) {
        pthread_mutex_unlock(&task->piece->mutex);
        return NULL;
    }
    task->piece->state = PIECE_DOWNLOADING;
    pthread_mutex_unlock(&task->piece->mutex);
    if (download_piece(task) == 0) {
        pthread_mutex_lock(&task->piece->mutex);
        task->piece->state = PIECE_COMPLETED;
        pthread_mutex_unlock(&task->piece->mutex);
    }
     else 
    {
        pthread_mutex_lock(&task->piece->mutex);
        task->piece->state = PIECE_FAILED;
        pthread_mutex_unlock(&task->piece->mutex);
    }
    return NULL;
}
int download_piece(DownloadTask *task)
{
    (void)task;
    return -1;
}
void *upload_worker(void *arg)
{
    (void)arg;
    return NULL;
}
int start_downloads( DownloadTask *tasks, size_t task_count, size_t thread_count)
{
    if (tasks == NULL || task_count == 0 || thread_count == 0) 
    {
        return -1;
    }
    if (thread_count > task_count)
     {
        thread_count = task_count;
    }
    pthread_t *threads = malloc(thread_count * sizeof(pthread_t));
    if (threads == NULL) 
    {
        return -1;
    }
    size_t started = 0;
    while (started < task_count) 
    {
        size_t batch = 0;
        while (batch < thread_count && started + batch < task_count) 
        {
            if (pthread_create(&threads[batch], NULL, download_worker, &tasks[started + batch]) != 0) 
                {
                break;
            }
            batch++;
        }
        for (size_t i = 0; i < batch; i++) 
        {
            pthread_join(threads[i], NULL);
        }
        if (batch == 0) 
        {
            free(threads);
            return -1;
        }
        started += batch;
    }
    free(threads);
    return 0;
}
