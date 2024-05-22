#ifndef PEER_H
#define PEER_H

#include <stdlib.h>
#include <pthread.h>
#include <arpa/inet.h>

struct peer_list {
    struct peer* head;
    struct peer* tail;
    pthread_mutex_t peerlist_lock;
    int length;
    int max_peers;
};

struct peer {
    struct sockaddr_in address;
    int sock_fd;
    pthread_t thread_id;
    struct peer_list* peer_list;
    struct peer* next;
    struct peer* previous;
};

struct peer_list* initiate_peers(int max_peers);
struct peer* add_peer(struct peer_list* list, int sock_fd, struct sockaddr_in address);
struct peer* get_peer(struct peer_list* list, char* ip, int port);
int remove_peer(struct peer_list* list, char* ip, int port);
int remove_peer_direct(struct peer* peer);
void print_peers(struct peer_list* list);
void free_peerlist(struct peer_list* list);
void end_free_peerlist(struct peer_list* list);


#endif