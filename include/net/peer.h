#ifndef PEER_H
#define PEER_H

#include <stdlib.h>
#include <pthread.h>

struct peer_list {
    struct peer* head;
    struct peer* tail;
    pthread_t peer_lock;
    int length;
};

struct peer {
    char* ip;
    int port;
    int sock_fd;
    struct peer* next;
};

int initiate_peers(int max_size, struct peer_list* all_peers);
void add_peer(struct peer_list* list, char* ip, int port, int sock_fd);
int remove_peer(struct peer_list* list, char* ip, int port);

#endif