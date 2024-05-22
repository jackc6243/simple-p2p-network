#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "../../include/net/config.h"
#include "../../include/net/packet.h"
#include "../../include/net/network.h"
#include "../../include/net/peer.h"
#include "../../include/net/packet.h"

#define TRUE 1
#define FALSE 0

// set timeout for socket
void set_socket_timeout(int sockfd, int seconds, int microseconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = microseconds;

    // Set receive timeout
    if (setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting receive timeout");
        exit(EXIT_FAILURE);
    }

    // Set send timeout
    if (setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("Error setting send timeout");
        exit(EXIT_FAILURE);
    }
}

int init_pthread(struct peer_list* peer_list, int peer_socket, struct sockaddr_in address) {
    // peer has been succesfully connected, now we add to peer_list
    struct peer* new_peer = add_peer(peer_list, peer_socket, address);
    printf("peer added to peer list, socket fd is %d, IP is : %s, port : %d\n",
        peer_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    // create a new thread for this peer
    if (pthread_create(&new_peer->thread_id, NULL, peer_thread, new_peer) != 0) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
        return FALSE;
    }

    return TRUE;
}

// creates a connection to the given ip and port, returns socket fd. Returns 1 if success, 0 if fail
int connect_new_peer(char* ip, int port, struct peer_list* peer_list) {
    int sock = 0;
    // Create socket file descriptor
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 0;
    }
    set_socket_timeout(sock, 2, 0); // set timeout to 2 seconds

    // Configure server address
    struct sockaddr_in peer_address;
    peer_address.sin_family = AF_INET;
    peer_address.sin_port = htons(port);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, ip, &peer_address.sin_addr) <= 0) {
        // perror("Invalid address/ Address not supported");
        close(sock);
        return 0;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr*)&peer_address, sizeof(peer_address)) < 0) {
        printf("Connect failed with error: %s\n", strerror(errno));
        close(sock);
        return 0;
    }
    puts("connection succesful\n");

    // making sure we follow the protocol of receiving ACP and sending ACK
    if (check_receive(sock, PKT_MSG_ACP) == NULL || send_packet(sock, PKT_MSG_ACK) == FALSE) {
        close(sock);
        return 0;
    }

    puts("initating thread now\n");

    // connection succesful, now we create a new thread for this peer socket
    init_pthread(peer_list, sock, peer_address);

    return sock;
}

// initiate connection with the new peer, create a new thread for this peer and also add the peer to peer_list
int accept_new_peer(int sock, struct sockaddr_in address, struct peer_list* peer_list) {
    printf("New connection, socket fd is %d, IP is : %s, port : %d\n",
        sock, inet_ntoa(address.sin_addr), ntohs(address.sin_port));

    printf("checking protocol of new peer\n");
    // making sure we follow the protocol of sending an ACP and receiving ACK
    if (send_packet(sock, PKT_MSG_ACP) == FALSE || check_receive(sock, PKT_MSG_ACK) == NULL) {
        close(sock);
        return 0;
    }

    printf("protocol is fine\n");
    // connection succesful, now we create a new thread for this peer socket
    init_pthread(peer_list, sock, address);

    return sock;
}

void peer_thread_cleanup(void* arg) {
    puts("cleaning up peer");
    struct peer* peer = (struct peer*)arg;
    end_peer(peer); // end peer connection
    remove_peer_direct(peer); // remove peer from peer_lists
}

void packet_cleanup(void* arg) {
    free(arg);
}

// thread for each peer
void* peer_thread(void* arg) {
    struct peer* peer = (struct peer*)arg;
    printf("peer thread initated with socket:%d,\n", peer->sock_fd);

    struct btide_packet* packet = create_packet(0);

    pthread_cleanup_push(peer_thread_cleanup, (void*)peer);
    pthread_cleanup_push(packet_cleanup, (void*)packet);
    while (1) {
        ssize_t nread = read(peer->sock_fd, packet, sizeof(packet));
        if (nread < 0) {
            continue; // handling failed to read
        }

        // looping throgh all possible packets that could have been received
        if (packet->msg_code == PKT_MSG_ACK) {
            // don't have to do anything here
        } else if (packet->msg_code == PKT_MSG_ACP) {
            // don't have to do anything here
        } else if (packet->msg_code == PKT_MSG_DSN) {
            printf("Packet message code is DSN\n");
            // this peer is disconnecting
            break;
        } else if (packet->msg_code == PKT_MSG_REQ) {
            // respond with appropriate requested chunk
            printf("Packet message code is REQ\n");
        } else if (packet->msg_code == PKT_MSG_RES) {
            // respond by updating the given chunk within our packages
            printf("Packet message code is RES\n");
        } else if (packet->msg_code == PKT_MSG_PNG) {
            // Got a ping message, needs to send pong back
            send_packet(peer->sock_fd, PKT_MSG_POG);
        } else if (packet->msg_code == PKT_MSG_POG) {
            // we do not have to do anything if we receive a random pong message
        } else {
            printf("Packet message code is unknown\n");
        }
    }
    puts("out of loop");

    // free resources and end peer connection, need to run twice for both of our cleanup functions
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    return;
}

// create a listening server fd
int create_listener(int port) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
        return 0;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
        return 0;
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
        return 0;
    }
    return server_fd;
}

// gracefully disconnect from a peer, does not remove from peer list
void end_peer(struct peer* peer) {
    // let the peer know that I am disconnecting
    send_packet(peer->sock_fd, PKT_MSG_DSN);
    close(peer->sock_fd);
}

int create_server(struct config* config, struct peer_list* peer_list, struct package_list* package_list, pthread_t* thread_id) {
    struct server_config* server_config = (struct server_config*)malloc(sizeof(struct server_config));
    server_config->config = config;
    server_config->peer_list = peer_list;
    server_config->package_list = package_list;

    // create a new thread for the server
    if (pthread_create(thread_id, NULL, main_server_thread, (void*)server_config)) {
        perror("pthread_create failed");
        exit(EXIT_FAILURE);
        return FALSE;
    }
    return TRUE;
}

void server_cleanup(void* args) {
    int server_fd = *(int*)args;
    if (server_fd > 0)
        close(server_fd);
}

void config_cleanup(void* arg) {
    free((struct server_config*)arg);
}

void* main_server_thread(void* args) {
    // unpacking args
    struct server_config* server_config = (struct server_config*)args;
    struct config* config = server_config->config;
    struct peer_list* peer_list = server_config->peer_list;
    // struct package_list* package_list = server_config->package_list;

    int new_socket = 0;
    struct sockaddr_in address;
    int server_fd = create_listener(config->port); // create a listening socket
    if (server_fd <= 0) {
        puts("failed to create listener");
        free(server_config);
        return;
    }
    printf("server thread running on port %d\n", config->port);

    // add cleanups
    pthread_cleanup_push(server_cleanup, (void*)&server_fd);
    pthread_cleanup_push(config_cleanup, (void*)server_config);

    // inf loop
    while (1) {
        socklen_t addrlen = sizeof(struct sockaddr);
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }

        // double checks the peer is connected via protocal and create a new thread if valid
        accept_new_peer(new_socket, address, peer_list);
    }

    // resources no longer needed, need to run twice for both cleanup functions
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    return;
}

// ping all peers
void ping_peers(struct peer_list* list) {
    struct peer* head = list->head;
    pthread_mutex_lock(&(list->peerlist_lock));
    while (head != NULL) {
        send_packet(head->sock_fd, PKT_MSG_PNG);
        head = head->next;
    }
    pthread_mutex_unlock(&(list->peerlist_lock));
}