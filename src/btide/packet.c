#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <string.h>

#include "../../include/net/packet.h"
#include "../../include/crypt/sha256.h"

#define TRUE 1
#define FALSE 0

#define MIN(x, y) ((x) < (y) ? (x) : (y))

// allocate memory for packet
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

// send a packet to a given socket, if failed return 0
int send_packet(int sock_fd, int msg) {
    struct btide_packet* packet = create_packet(msg);
    return send_packet_direct(sock_fd, packet);
}

// send a packet directly given the packet
int send_packet_direct(int sock_fd, struct btide_packet* packet) {
    ssize_t nwritten;
    nwritten = send(sock_fd, packet, sizeof(packet), 0);
    if (nwritten <= 0) {
        fprintf(stderr, "could not write entire message to client: socket fd %d\n", sock_fd);
        free(packet);
        return 0; // failed
    }
    free(packet);

    return 1;
}

// check if we received a packet with message
struct btide_packet* check_receive(int sock_fd, int msg) {
    struct btide_packet* packet = create_packet(0);
    ssize_t nread = read(sock_fd, packet, sizeof(packet));

    // must check we have received ACP back
    if (nread <= 0 || packet->msg_code != msg) {
        fprintf(stderr, "Client disconnected: socket %d\n", sock_fd);
        free(packet);
        return NULL; // failed
    }
    free(packet);
    return packet;
}

// will send a ping and wait for a pong
int ping_pong(int sock_fd) {
    struct btide_packet* packet = create_packet(PKT_MSG_PNG);
    ssize_t nwritten;
    // make sure we send out a ping
    nwritten = send(sock_fd, packet, sizeof(packet), 0);
    if (nwritten <= 0) {
        fprintf(stderr, "could not write entire message to client: socket fd %d\n", sock_fd);
        free(packet);
        close(sock_fd);
        return 0; // failed
    }

    ssize_t nread = read(sock_fd, packet, sizeof(packet)); // we might have to make this non blocking so that we don't have an idle thread.
    // must check we have received pong back
    if (nread <= 0 || packet->msg_code != PKT_MSG_POG) {
        fprintf(stderr, "Client disconnected: socket fd %d\n", sock_fd);
        free(packet);
        close(sock_fd);
        return 0; // failed
    }
    free(packet);
    return 1;
}

// send a request packet for a particular chunk
int send_req(struct peer* peer, struct package* package, struct merkle_tree_node* chunk_node) {
    struct btide_packet* packet = init_req_packet(package->bpkg->ident, chunk_node->expected_hash, (uint32_t)chunk_node->chunk_size, chunk_node->offset);
    return send_packet_direct(peer->sock_fd, packet);
}

// parse a request packet and send the appropriate res packets
int parse_req(struct btide_packet* packet, struct package_list* list, struct peer* peer) {
    struct req_packet* req = &(packet->payload.req_packet);

    // find the package from ident
    struct package* package = get_package(list, req->ident);
    if (package == NULL) {
        return 0;
    }

    // create packet for sending
    struct btide_packet* output_packet = create_packet(PKT_MSG_RES);

    // find the chunk requested by the expected hash
    struct merkle_tree_node* chunk_node = find_chunk(package->bpkg->tree->root, req->chunk_hash, req->file_offset);

    // if i don't have the chunk or the chunk is incomplete, send back a res packet with error set to 1
    if (chunk_node == NULL || strncmp(chunk_node->computed_hash, chunk_node->expected_hash, 64) != 0) {
        output_packet->error = 1;
        send_packet_direct(peer->sock_fd, output_packet);
        free(output_packet);
        return 1;
    }

    // open the file
    FILE* file = fopen(package->bpkg->full_path, "rb");
    if (file == NULL) {
        puts("Error opening file");
        free(output_packet);
        return 0;
    }

    // Move the file pointer to the desired offset
    if (fseek(file, req->file_offset, SEEK_SET) != 0) {
        puts("Error seeking in file");
        fclose(file);
        free(output_packet);
        return 0;
    }

    // repeatedly send out res packets
    int chunk_offset = 0;
    int data_size;
    while (req->data_len > chunk_offset) {
        data_size = MIN(MAX_DATA_SIZE, req->data_len - chunk_offset);

        // reading the data from file
        uint16_t bytes_read = fread(output_packet->payload.res_packet.data, 1, data_size, file);
        if (bytes_read != data_size) {
            puts("Error reading file");
            break;
        }
        // setting the res packet
        set_res_packet(output_packet, NULL, package->bpkg->ident, chunk_node->computed_hash, data_size, chunk_offset);
        send_packet_direct(peer->sock_fd, output_packet); // sending the packet

        chunk_offset += MAX_DATA_SIZE;
    }

    free(output_packet);
    return 1;
}

// parse the packet and updates the file
int parse_res(struct btide_packet* packet, struct package_list* list) {
    // no need to parse if peer doesn't have the chunk
    if (packet->error > 0) {
        return FALSE;
    }
    struct res_packet* res = &(packet->payload.res_packet);

    // find that package exists
    struct package* package = get_package(list, res->ident);
    if (package == NULL) {
        return 0;
    }

    // find the chunk hash so that we can find the file offset
    struct merkle_tree_node* chunk_node = find_chunk(package->bpkg->tree->root, res->chunk_hash, -1); // last parameter is -1 because we don't know the file offset
    if (chunk_node == NULL) {
        return 0;
    }

    // open the file
    FILE* file = fopen(package->bpkg->full_path, "r+b");
    if (file == NULL) {
        puts("Error opening file");
        return 0;
    }

    // create packet for sending
    struct btide_packet* output_packet = create_packet(0);

    // Move the file pointer to the required chunk + the offset in the chunk
    if (fseek(file, chunk_node->offset + res->file_offset, SEEK_SET) != 0) {
        puts("Error seeking in file");
        fclose(file);
        free(output_packet);
        return 0;
    }

    // Write the data to the file, since we are writing data, we need to lock to ensure no race condition occurs
    pthread_mutex_lock(&package->p_lock);
    size_t bytes_written = fwrite(res->data, 1, res->data_len, file);
    if (bytes_written != res->data_len) {
        puts("Error writing to file");
        fclose(file);
        free(output_packet);
        return 0;
    }

    // Update the merkletree as this potentially change the completion state
    // First move the file pointer to the beginning of chunk node
    if (fseek(file, chunk_node->offset, SEEK_SET) != 0) {
        puts("Error seeking in file");
        fclose(file);
        free(output_packet);
        return 0;
    }
    // Now we upate the hash
    sha256_file_hash(file, chunk_node->chunk_size, chunk_node->computed_hash);
    pthread_mutex_lock(&package->p_lock);

    // Close the file and free resources
    fclose(file);
    free(output_packet);
    return 1;
}

struct btide_packet* init_req_packet(char* ident, char* hash, uint32_t data_len, uint32_t offset) {
    struct btide_packet* packet = (struct btide_packet*)malloc(sizeof(struct btide_packet));
    packet->msg_code = PKT_MSG_REQ;
    packet->payload.req_packet.file_offset = offset;
    packet->payload.req_packet.data_len = data_len;
    memcpy(packet->payload.req_packet.chunk_hash, hash, CHUNK_HASH_SIZE);
    memcpy(packet->payload.req_packet.ident, ident, IDENT_SIZE);
    return packet;
}

// set fields in the res packet
struct btide_packet* set_res_packet(struct btide_packet* packet, char* data, char* ident, char* hash, uint16_t data_len, uint32_t offset) {
    packet->msg_code = PKT_MSG_RES;
    packet->payload.res_packet.file_offset = offset;
    packet->payload.res_packet.data_len = data_len;
    if (data != NULL) {
        memcpy(packet->payload.res_packet.data, data, data_len);
    }
    memcpy(packet->payload.res_packet.chunk_hash, hash, CHUNK_HASH_SIZE);
    memcpy(packet->payload.res_packet.ident, ident, IDENT_SIZE);
    return packet;
}