#include <stdlib.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>

#include "../../include/net/packet.h"

#define TRUE 1
#define FALSE 0

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
    ssize_t nwritten;
    nwritten = send(sock_fd, packet, sizeof(packet), 0);
    if (nwritten <= 0) {
        fprintf(stderr, "could not write entire message to client: socket fd %d\n", sock_fd);
        free(packet);
        return 0; // failed
    }
    free(packet);
    printf("sent packet: %d\n", msg);

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
    printf("received packet: %d\n", msg);
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