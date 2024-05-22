#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include "peer.h"

#define BUFFER_SIZE 1024

struct server_config {
    struct config* config;
    struct peer_list* peer_list;
    struct package_list* package_list;
};

// Function to create a listener socket
int init_pthread(struct peer_list* peer_list, int peer_socket, struct sockaddr_in address);
int connect_new_peer(char* ip, int port, struct peer_list* peer_list);
int accept_new_peer(int sock, struct sockaddr_in address, struct peer_list* peer_list);
int create_listener(int port);
void end_peer(struct peer* peer);
void* peer_thread(void* arg);
int create_server(struct config* config, struct peer_list* peer_list, struct package_list* package_list, pthread_t* thread_id);
void* main_server_thread(void* args);

#endif // SERVER_H