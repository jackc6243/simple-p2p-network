#ifndef SERVER_H
#define SERVER_H

#include <netinet/in.h>
#include "peer.h"

#define BUFFER_SIZE 1024

struct server_config {
    struct config* config;
    struct peer_list* peer_list;
    struct package_list* package_list;
    struct peer* peer;
};

// Function to create a listener socket
int init_pthread(struct server_config* server_config, int peer_socket, struct sockaddr_in address);
int connect_new_peer(char* ip, int port, struct server_config* server_config);
int accept_new_peer(int sock, struct sockaddr_in address, struct server_config* server_config);
int create_listener(int port);
void end_peer(struct peer* peer);
void* peer_thread(void* arg);
int create_server(struct server_config* server_config, pthread_t* thread_id);
struct server_config* setup_config(struct config* config, struct peer_list* peer_list, struct package_list* package_list);
void* main_server_thread(void* args);
void ping_peers(struct peer_list* list);
void server_config_destroy(struct server_config* server_config);

#endif // SERVER_H