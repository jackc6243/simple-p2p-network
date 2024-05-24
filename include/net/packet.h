#ifndef NETPKT_H
#define NETPKT_H

#include <stdint.h>
#include "../tree/merkletree.h"
#include "./package.h"
#include "./peer.h"
#include "../../include/chk/pkgchk.h"

#define IDENT_SIZE (1024)
#define OFFSET_SIZE (4)
#define MAX_DATA_SIZE (2998)
#define CHUNK_HASH_SIZE (64)

#define PKT_MSG_ACK 0x0c
#define PKT_MSG_ACP 0x02
#define PKT_MSG_DSN 0x03
#define PKT_MSG_REQ 0x06
#define PKT_MSG_RES 0x07
#define PKT_MSG_PNG 0xFF
#define PKT_MSG_POG 0x00


struct res_packet {
    uint32_t file_offset;
    uint16_t data_len;
    char data[MAX_DATA_SIZE];
    char chunk_hash[CHUNK_HASH_SIZE];
    char ident[IDENT_SIZE];
};

struct req_packet {
    uint32_t file_offset;
    uint32_t data_len;
    char chunk_hash[CHUNK_HASH_SIZE];
    char ident[IDENT_SIZE];
};

union btide_payload {
    struct res_packet res_packet;
    struct req_packet req_packet;
};

struct btide_packet {
    uint16_t msg_code;
    uint16_t error;
    union btide_payload payload;
};

struct btide_packet* create_packet(int message);
int send_packet(int sock_fd, int msg);
struct btide_packet* check_receive(int sock_fd, int msg);
int ping_pong(int sock_fd);
struct btide_packet* init_req_packet(char* ident, char* hash, uint32_t data_len, uint32_t offset);
struct btide_packet* set_res_packet(struct btide_packet* packet, char* data, char* ident, char* hash, uint16_t data_len, uint32_t offset);
int send_packet_direct(int sock_fd, struct btide_packet* packet);
int send_req(struct peer* peer, struct package* package, struct merkle_tree_node* chunk_node);
int parse_res(struct btide_packet* packet, struct package_list* list);
int parse_req(struct btide_packet* packet, struct package_list* list, struct peer* peer);

#endif
