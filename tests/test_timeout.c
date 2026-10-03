#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <time.h>

#include "../src/recovery/timeout.h"

static void wait_ms(long milliseconds)
{
    struct timespec request;

    request.tv_sec = milliseconds / 1000;
    request.tv_nsec = (milliseconds % 1000) * 1000000L;

    nanosleep(&request, NULL);
}

int main(void)
{
    Timeout timer;

    printf("Starting timeout test...\n");

    timeout_start(&timer, 100);

    printf("Checking before timeout...\n");

    if (timeout_expired(&timer)) {
        printf("FAIL: Timer expired too early\n");
        return 1;
    }

    wait_ms(150);

    printf("Checking after timeout...\n");

    if (!timeout_expired(&timer)) {
        printf("FAIL: Timer did not expire\n");
        return 1;
    }

    printf("PASS: Timeout detected correctly\n");

    timeout_reset(&timer);

    if (timeout_expired(&timer)) {
        printf("FAIL: Timer did not reset correctly\n");
        return 1;
    }

    printf("PASS: Timeout reset correctly\n");
    printf("Timeout test completed successfully.\n");

    return 0;
}
