#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "../../include/chk/pkgchk.h"
#include "../../include/net/peer.h"
#include "../../include/net/packet.h"

#include <pthread.h>

// initiate new peer
struct peer_list* initiate_peers(int max_peers) {
    struct peer_list* all_peers = (struct peer_list*)malloc(sizeof(struct peer_list));
    all_peers->head = NULL;
    all_peers->tail = NULL;
    all_peers->length = 0;
    all_peers->max_peers = max_peers;
    // mutex to prevent multiple threads from changing peer list
    if (pthread_mutex_init(&all_peers->peerlist_lock, NULL) != 0) {
        perror("Can't initate peer mutex");
        free(all_peers);
        return NULL;
    }
    return all_peers;
}

// Add a new peer to the end of the list
struct peer* add_peer(struct peer_list* list, int sock_fd, struct sockaddr_in address) {
    struct peer* new_peer = (struct peer*)malloc(sizeof(struct peer));
    new_peer->address = address;
    new_peer->sock_fd = sock_fd;
    new_peer->peer_list = list;
    new_peer->next = NULL;
    new_peer->previous = list->tail; // set previous to current tail

    pthread_mutex_lock(&(list->peerlist_lock));
    if (list->head == NULL) {
        list->head = new_peer; // empty list case, set new_peer as head
    } else {
        list->tail->next = new_peer; // if tail exists, set its next to new_peer
    }

    list->tail = new_peer; // new_peer is always going to be tail
    list->length++;
    pthread_mutex_unlock(&(list->peerlist_lock));
    return new_peer;
}

// get a peer if it exists, otherwise return NULL
struct peer* get_peer(struct peer_list* list, char* ip, int port) {
    pthread_mutex_lock(&(list->peerlist_lock));
    struct peer* current = list->head;

    while (current != NULL) {
        if (strcmp(inet_ntoa(current->address.sin_addr), ip) == 0
            && current->address.sin_port == ntohs(port)) {
            pthread_mutex_unlock(&(list->peerlist_lock));
            return current;
        }
        current = current->next;
    }

    pthread_mutex_unlock(&(list->peerlist_lock));
    return NULL;
}

// Remove a peer from the list by first finding it, then removing it directly
// the port and ip are both in local form
int remove_peer(struct peer_list* list, char* ip, int port) {
    struct peer* peer = get_peer(list, ip, port);
    if (peer != NULL) {
        if (remove_peer_direct(peer)) {
            return 1;
        }
    }
    return 0;
}

// remove peer directly
int remove_peer_direct(struct peer* peer) {
    struct peer_list* list = peer->peer_list;
    pthread_mutex_lock(&(list->peerlist_lock));

    // edge case where this peer is the head
    if (list->head == peer) {
        list->head = peer->next;
        if (list->head != NULL) { // check if list is not empty after removal
            list->head->previous = NULL;
        }
    } else {
        peer->previous->next = peer->next;
        if (peer->next != NULL) { // check if peer is not the last node
            peer->next->previous = peer->previous;
        }
    }

    // case where peer the tail
    if (peer == list->tail) {
        list->tail = peer->previous;
        if (list->tail != NULL) { // check if list is not empty after removal
            list->tail->next = NULL;
        }
    }

    free(peer);
    list->length--;
    pthread_mutex_unlock(&(list->peerlist_lock));
    return 1;
}

// print all peers
void print_peers(struct peer_list* list) {
    struct peer* head = list->head;
    pthread_mutex_lock(&(list->peerlist_lock));
    int i = 1;
    while (head != NULL) {
        printf("%d. %s:%d\n", i, inet_ntoa(head->address.sin_addr), ntohs(head->address.sin_port));
        head = head->next;
        i++;
    }
    pthread_mutex_unlock(&(list->peerlist_lock));
}

// no need to use mutex since we are only only to free when we are exiting the program where all other threads are forced to quit
void free_peerlist(struct peer_list* list) {
    struct peer* current = list->head;
    struct peer* next_peer;

    while (current != NULL) {
        next_peer = current->next;
        free(current);
        current = next_peer;
    }

    list->head = NULL;
    list->tail = NULL;
    list->length = 0;

    free(list);
}

// end all connections and free the peerlist
void end_free_peerlist(struct peer_list* list) {
    struct peer* current = list->head;
    struct peer* next_peer;

    while (current != NULL) {
        next_peer = current->next;

        // letting other peers know we are disconnecting
        send_packet(current->sock_fd, PKT_MSG_DSN);
        close(current->sock_fd);

        free(current);
        current = next_peer;
    }

    list->head = NULL;
    list->tail = NULL;
    list->length = 0;

    free(list);
}