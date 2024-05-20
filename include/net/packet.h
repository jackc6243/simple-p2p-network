#ifndef NETPKT_H
#define NETPKT_H

#include <stdint.h>

#define PAYLOAD_MAX (4092)
#define IDENT_SIZE (1024)
#define CHUNK_HASH_SIZE (64)
#define OFFSET_SIZE (4)
#define DATA_LEN_SIZE (2)
#define MAX_DATA_PACK_SIZE (2998)

#define PKT_MSG_ACK 0x0c
#define PKT_MSG_ACP 0x02
#define PKT_MSG_DSN 0x03
#define PKT_MSG_REQ 0x06
#define PKT_MSG_RES 0x07
#define PKT_MSG_PNG 0xFF
#define PKT_MSG_POG 0x00


struct data_packet {
    uint8_t file_offset[OFFSET_SIZE];
    uint8_t data[MAX_DATA_PACK_SIZE];
    uint8_t data_len[DATA_LEN_SIZE];
    uint8_t ident[IDENT_SIZE];
}__attribute__((packed));

union btide_payload {
    uint8_t data[PAYLOAD_MAX];
    struct data_packet packet_res_req;
};

struct btide_packet {
    uint16_t msg_code;
    uint16_t error;
    union btide_payload payload;
};

struct btide_packet* create_packet(int message);


#endif
