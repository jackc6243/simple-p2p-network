#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>

#include "../../include/net/packet.h"

struct btide_packet* create_packet(int message) {
    struct btide_packet* packet = (struct btide_packet*)malloc(sizeof(struct btide_packet));

    if (packet == NULL) {
        perror("Failed to malloc btide_packet");
        return NULL;
    }

    packet->msg_code = message;
    packet->error = 0;
    return packet;
}

