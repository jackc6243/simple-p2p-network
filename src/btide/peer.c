#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../../include/chk/pkgchk.h"
#include "../../include/net/state.h"
#include <pthread.h>

void initiate_peers(struct peer_list* all_peers) {
    all_peers->head = NULL;
    all_peers->length = 0;
}

// Add a new peer to the end of the list
void add_peer(struct peer_list* list, char* ip, int port, int sock_fd) {
    struct peer* new_peer = (struct peer*)malloc(sizeof(struct peer));
    new_peer->ip = ip;
    new_peer->port = port;
    new_peer->sock_fd = sock_fd;
    new_peer->next = NULL;

    pthread_mutex_lock(&(list->peer_lock));
    if (list->head == NULL) {
        list->head = new_peer;
        list->tail = new_peer;
    } else {
        list->tail->next = new_peer;
        list->tail = new_peer;
    }
    list->length++;
    pthread_mutex_unlock(&(list->peer_lock));
}

// Remove a peer from the list, mutex to prevent us from removing two peers at once
int remove_peer(struct peer_list* list, char* ip, int port) {
    pthread_mutex_lock(&(list->peer_lock));
    struct peer* current = list->head;
    struct peer* previous = NULL;

    while (current != NULL) {
        if (strcmp(current->ip, ip) == 0 && current->port == port) {
            if (previous == NULL) {
                list->head = current->next;
            } else {
                previous->next = current->next;
            }

            if (current == list->tail) {
                list->tail = previous;
            }

            free(current);
            list->length--;
            pthread_mutex_unlock(&(list->peer_lock));
            return 1;
        }

        previous = current;
        current = current->next;
    }
    pthread_mutex_unlock(&(list->peer_lock));
    return 0
}

void print_peers(struct peer_list* list) {
    struct peer* head = list->head;
    pthread_mutex_lock(&(list->peer_lock));
    int i = 0;
    while (head != NULL) {
        printf("%d. %s:%d", i, head->ip, head->port);
        i++;
    }
    pthread_mutex_unlock(&(list->peer_lock));
}