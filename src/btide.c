#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "../include/chk/pkgchk.h"
#include "../include/net/config.h"
#include "../include/net/network.h"
#include "../include/net/peer.h"
#include "../include/net/packet.h"

#define MAX_THREADS 2048
#define MAX_COMMAND 5540
#define MAX_IP_LENGTH 30

#define TRUE 1
#define FALSE 0

volatile int terminate = 0;
pthread_t server_thread;
pthread_mutex_t peer_mutex;
struct peer_list* peer_list;

// handle exit more gracefully
void sigint_handler(int signum) {
    printf("\nReceived SIGINT (Ctrl + C). Quitting gracefully...\n");
    end_free_peerlist(peer_list);
    exit(signum); // Exit the program with the signal number
}

// saves the ip and port into the given addresses
int get_ip_port(char** context, char* ip, int* port) {
    char delim[] = ":\n";
    char* tok;

    // obtaining ip
    tok = strtok_r(NULL, delim, context);
    if (tok == NULL) {
        return FALSE;
    }
    strcpy(ip, tok);

    // obtaining port number
    tok = strtok_r(NULL, delim, context);
    if (sscanf(tok, "%d", port) != 1) {
        return FALSE;
    }

    return TRUE;
}

int main(int argc, char** argv) {
    // config file path not given
    if (argc < 2) {
        puts("Please enter config file");
        return 0;
    }

    // parsing config file
    struct config* config = parse_config(argv[1]);
    if (config == NULL) {
        puts("No such config file");
        return 0;
    }

    // config file is not in correct format
    if (config->status > 2) {
        exit(config->status);
    }

    printf("direct:%s, max_peers:%d, port:%d\n", config->directory, config->max_peers, config->port);

    // initating some needed variables and buffers
    char* tok; char* context;
    char delim[] = " \n";
    char ip[MAX_IP_LENGTH] = { 0 };
    // struct sockaddr_in temp_addr;
    // int temp;
    struct peer* temp_peer;
    int port;
    char input[MAX_COMMAND];

    // setup packages, peer_list and listening server
    peer_list = initiate_peers(config->max_peers);
    create_server(config, peer_list, NULL, &server_thread);

    while (1) {
        // Read input from stdin
        if (fgets(input, MAX_COMMAND, stdin) == NULL) {
            continue;
        }
        tok = strtok_r(input, delim, &context);

        // Parsing commands
        if (strcmp(input, "QUIT") == 0) {
            printf("Exiting...\n");
            terminate = 1;
            end_free_peerlist(peer_list); // terminate all peer connections
            break;
        } else if (strcmp(input, "CONNECT") == 0) {
            // handle incorrect arguments
            if (get_ip_port(&context, ip, &port) == FALSE) {
                printf("Missing address and port argument\n");
                continue;
            }

            // handling if peer already exists
            if (get_peer(peer_list, ip, port) != NULL) {
                printf("Already connected to peer\n");
                continue;
            }

            // cannot have more than the max amount of peers
            pthread_mutex_lock(&(peer_list->peerlist_lock));
            if (peer_list->length >= peer_list->max_peers) {
                pthread_mutex_unlock(&(peer_list->peerlist_lock));
                printf("Reached max number of peers\n");
                continue;
            }
            pthread_mutex_unlock(&(peer_list->peerlist_lock));

            printf("trying to connect to peer %s:%d\n", ip, port);
            // attempting to connect to peer
            if (connect_new_peer(ip, port, peer_list)) {
                printf("Connection established with peer.\n");
            } else {
                printf("Unable to connect to request peer\n");
            }

        } else if (strcmp(input, "DISCONNECT") == 0) {
            // handle incorrect arguments
            if (get_ip_port(&context, ip, &port) == FALSE) {
                printf("Missing address and port argument\n");
                continue;
            }

            // Can't disconnect from peer if it was never connected
            temp_peer = get_peer(peer_list, ip, port);
            if (temp_peer == NULL) {
                printf("Unknown peer, not connected\n");
                continue;
            }

            // send signal to peer thread to end
            pthread_cancel(temp_peer->thread_id);

            printf("Disconnected from peer\n");
        } else if (strcmp(input, "ADDPACKAGE") == 0) {
            printf("Adding package...\n");
            // Add your add package logic here
        } else if (strcmp(input, "REMPACKAGE") == 0) {
            printf("Removing package...\n");
            // Add your remove package logic here
        } else if (strcmp(input, "PACKAGES") == 0) {
            printf("Listing packages...\n");
            // Add your list packages logic here
        } else if (strcmp(input, "PEERS") == 0) {
            printf("Connected to:\n");
            ping_peers(peer_list);
            print_peers(peer_list);
        } else if (strcmp(input, "FETCH") == 0) {
            printf("Fetching...\n");
            // Add your fetch logic here
        } else {
            printf("Invalid input.\n");
        }
    }

    // freeing resources
    pthread_mutex_destroy(&peer_mutex);
    free(config->directory);
    free(config);
}
