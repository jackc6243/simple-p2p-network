#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>

#include "../../include/net/config.h"
#include "../../include/net/packet.h"
#include "../../include/net/server.h"

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void sigint_handler(int signum) {
    printf("\nReceived SIGINT (Ctrl + C). Quitting...\n");
    exit(signum); // Exit the program with the signal number
}

int create_listener(int port) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket creation failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return -1;
    }
    if (listen(server_fd, 3) < 0) {
        perror("Listen failed");
        return -1;
    }

    return server_fd;
}

int process_new_peer(int peer_socket, struct sockaddr_in address) {
    printf("New connection, socket fd is %d, IP is : %s, port : %d\n",
        peer_socket, inet_ntoa(address.sin_addr), ntohs(address.sin_port));


    // read message
    // char buffer[BUFFER_SIZE];
    struct btide_packet* packet = create_packet(0);
    ssize_t nread = read(peer_socket, packet, BUFFER_SIZE);
    if (nread <= 0) {
        fprintf(stderr, "Client disconnected: socket fd %d\n", peer_socket);
        close(peer_socket);
        peer_socket = 0;
        return -1; // failed
    }

    // send message back
    ssize_t nwritten;
    packet->msg_code = PKT_MSG_ACP;
    nwritten = send(peer_socket, packet, sizeof(packet), 0);
    if (nwritten <= 0) {
        fprintf(stderr, "could not write entire message to client: socket fd %d\n", peer_socket);
        close(peer_socket);
        peer_socket = 0;
        return -1; // failed (but there could be a good reason to try again!)
    }

    return peer_socket;
}

int main_server(struct config* config) {

    // allow quit via ctrl-C
    if (signal(SIGINT, sigint_handler) == SIG_ERR) {
        perror("Error registering signal handler for SIGINT");
        return 1;
    }
    int server_fd = create_listener(config->port);

    // inf loop
    int new_socket;
    struct sockaddr_in address;
    while (1) {
        printf("waiting for connections\n");

        socklen_t addrlen = sizeof(struct sockaddr);
        if ((new_socket = accept(server_fd, (struct sockaddr*)&address, (socklen_t*)&addrlen)) < 0) {
            perror("Accept failed");
            exit(EXIT_FAILURE);
        }
        process_new_peer(new_socket, address);
    }

    // resources no longer needed
    if (new_socket > 0)
        close(new_socket);
    close(server_fd);

    return 0;
}