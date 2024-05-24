#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>

#include "../include/chk/pkgchk.h"
#include "../include/tree/merkletree.h"
#include "../include/net/config.h"
#include "../include/net/network.h"
#include "../include/net/peer.h"
#include "../include/net/packet.h"
#include "../include/net/package.h"

#define MAX_THREADS 2048
#define MAX_COMMAND 5540
#define MAX_IP_LENGTH 30

#define TRUE 1
#define FALSE 0

volatile int terminate = 0;
pthread_t server_thread;
pthread_mutex_t peer_mutex;
struct peer_list* peer_list;
struct package_list* package_list;

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

// return 0 if failed, otherwise return 1 if succesful parsing and 2 if succesful parsing and offset is parsed
int get_fetch_args(char** context, char* ip, int* port, char* ident, char* hash, int* offset) {
    char delim[] = " \n";
    char* tok;

    // getting ip and port
    if (get_ip_port(context, ip, port) == 0) {
        return FALSE;
    }

    // getting ident
    tok = strtok_r(NULL, delim, context);
    if (tok == NULL) {
        return FALSE;
    }
    strcpy(ident, tok);

    // getting hash
    tok = strtok_r(NULL, delim, context);
    if (tok == NULL) {
        return FALSE;
    }
    strcpy(hash, tok);

    // parsing offset optional
    tok = strtok_r(NULL, delim, context);
    if (tok != NULL) {
        if (sscanf(tok, "%d", offset) == 1) {
            return 2;
        }
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

    // initating some variables and buffers for repeated use
    char* tok; char* context;
    char delim[] = " \n";
    char ident[MAX_IDENT] = { 0 };
    char hash[65] = { 0 };
    char ip[MAX_IP_LENGTH] = { 0 };
    int port; int temp;
    char input[MAX_COMMAND];
    struct package* temp_package;
    struct peer* temp_peer;
    struct bpkg_obj* bpkg;
    struct bpkg_query* query;
    struct merkle_tree_node* chunk_node;

    // setup packages, peer_list, configurations and listening server
    package_list = initiate_packages();
    peer_list = initiate_peers(config->max_peers);
    struct server_config* server_config = setup_config(config, peer_list, package_list);
    create_server(server_config, &server_thread);

    while (1) {
        // Read input from stdin
        if (fgets(input, MAX_COMMAND, stdin) == NULL) {
            continue;
        }
        tok = strtok_r(input, delim, &context);

        // Parsing commands
        if (strcmp(input, "QUIT") == 0) {
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

            // printf("trying to connect to peer %s:%d\n", ip, port);
            // attempting to connect to peer
            if (connect_new_peer(ip, port, server_config)) {
                printf("Connection established with peer\n");
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
            // get file path
            tok = strtok_r(NULL, delim, &context);
            if (tok == NULL) {
                puts("Missing file argument");
                continue;
            }

            // loading the bpkg file
            bpkg = bpkg_load(config->directory, tok);
            if (bpkg == NULL) {
                puts("Unable to parse bpkg file");
                continue;
            }

            query = bpkg_file_check(bpkg);
            if (query == NULL) {
                puts("Cannot open file");
                continue;
            }

            temp = 0; // temp tells us the completion state of this file
            if (strcmp(query->hashes[0], "File Exists") == 0) {
                // we should update the merkle tree in the file
                query = bpkg_get_min_completed_hashes(bpkg);
                if (query->len == 1 &&
                    strncmp(query->hashes[0], bpkg->tree->all_nodes[0]->expected_hash, 64) == 0) {
                    // file is completed
                    temp = 1;
                }
            }

            bpkg_query_destroy(query);
            add_package(package_list, bpkg, temp);

        } else if (strcmp(input, "REMPACKAGE") == 0) {
            tok = strtok_r(NULL, delim, &context);
            if (tok == NULL || strlen(tok) < 20) {
                puts("Missing identifier argument, please specify whole 1024 character or at least 20 characters.");
                continue;
            }

            if (remove_package(package_list, tok) == 0) {
                // couldn't remove the package
                puts("Identifier provided does not match managed packages");
            }

            puts("Package has been removed");

        } else if (strcmp(input, "PACKAGES") == 0) {
            pthread_mutex_lock(&package_list->plist_lock);
            if (package_list->length > 0) {
                print_packages(package_list);
            } else {
                puts("No packages managed");
            }
            pthread_mutex_unlock(&package_list->plist_lock);

        } else if (strcmp(input, "PEERS") == 0) {
            if (peer_list->length > 0) {
                printf("Connected to:\n");
                ping_peers(peer_list);
                print_peers(peer_list);
            }
            // puts("Not connected to any peers");
        } else if (strcmp(input, "FETCH") == 0) {
            printf("Fetching...\n");
            temp = -1; // by default we set no offset

            // not all args were parsed
            if (get_fetch_args(&context, ip, &port, ident, hash, &temp) == 0) {
                puts("Missing arguments from command");
            }

            // Trying to find peer
            temp_peer = get_peer(peer_list, ip, port);
            if (temp_peer == NULL) {
                printf("Unable to request chunk, peer not in list \n");
                continue;
            }

            // tring to find the package
            temp_package = get_package(package_list, ident);
            if (temp_package == NULL) {
                printf("Unable to request chunk, package is not managed\n");
                continue;
            }

            // trying to get all chunk hashes
            chunk_node = find_chunk(bpkg->tree->root, hash, temp);
            if (chunk_node == NULL) {
                puts("Unable to request chunk, chunk hash does not belong to package");
                continue;
            }

            // send the request
            send_req(temp_peer, temp_package, chunk_node);
        } else {
            puts("Invalid input.");
        }

        // reset some variables
        query = NULL;
        temp_peer = NULL;
        temp_package = NULL;
        chunk_node = NULL;
        bpkg = NULL;
        temp = 0;
        port = 0;
    }

    // freeing resources
    pthread_mutex_destroy(&peer_mutex);
    free(config->directory);
    free(config);
}
