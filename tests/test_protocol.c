#include "../src/common/protocol.h"

#include <stdio.h>

int main(void)
{
    printf("Protocol test started.\n");

    printf("MSG_HELLO = %d\n", MSG_HELLO);
    printf("MSG_PEER_LIST = %d\n", MSG_PEER_LIST);
    printf("MSG_PIECE_REQUEST = %d\n", MSG_PIECE_REQUEST);
    printf("MSG_PIECE_RESPONSE = %d\n", MSG_PIECE_RESPONSE);

    printf("Protocol definitions are accessible.\n");

    return 0;
}