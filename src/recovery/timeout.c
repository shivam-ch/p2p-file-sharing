#define _POSIX_C_SOURCE 200809L
#include "timeout.h"
#include <stddef.h>
#include <time.h>

static uint64_t current_time_ms(void)
{
    struct timespec ts;

    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }

    return ((uint64_t)ts.tv_sec * 1000ULL) +
           ((uint64_t)ts.tv_nsec / 1000000ULL);
}

void timeout_start(
    Timeout *timer,
    uint32_t timeout_ms
)
{
    if (timer == NULL) {
        return;
    }

    timer->timeout_ms = timeout_ms;
    timer->start_time_ms = current_time_ms();
}

int timeout_expired(
    const Timeout *timer
)
{
    uint64_t now;

    if (timer == NULL) {
        return 0;
    }

    now = current_time_ms();

    return (now - timer->start_time_ms) >= timer->timeout_ms;
}

void timeout_reset(
    Timeout *timer
)
{
    if (timer == NULL) {
        return;
    }

    timer->start_time_ms = current_time_ms();
}
