#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../../include/chk/pkgchk.h"
#include "../../include/net/state.h"
#include <pthread.h>


int initiate_packages() {
    struct package_list* all_packages = (struct package_list*)malloc(sizeof(struct package_list));
    all_packages->head = NULL;
    all_packages->tail = NULL;
    all_packages->length = 0;
}

// Add a new package to the end of the list
void add_package(struct package_list* list, struct bpkg_obj* bpkg, int is_completed) {
    struct package* new_package = (struct package*)malloc(sizeof(struct package));
    new_package->bpkg = bpkg;
    new_package->is_completed = is_completed;
    new_package->next = NULL;
    new_package->previous = NULL;
    new_package->package_list = list;

    if (pthread_mutex_init(&new_package->p_lock, NULL) != 0) {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }

    if (list->head == NULL) {
        list->head = new_package;
        list->tail = new_package;
    } else {
        list->tail->next = new_package;
        list->tail = new_package;
    }
    list->length++;
}

// Remove a package from the list
int remove_package(struct package_list* list, char* ident) {
    struct package* current = list->head;
    struct package* previous = NULL;

    while (current != NULL) {
        if (strncmp(current->bpkg->ident, ident, 20) == 0) {
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
            return 1;
        }

        previous = current;
        current = current->next;
    }
    return 0;
}

// print out all packages
void print_packages(struct package_list* list) {
    struct package* head = list->head;
    int i = 1;
    char complete[] = "COMPLETED";
    char incomplete[] = "INCOMPLETE";

    char* temp;
    while (head != NULL) {
        temp = head->is_complete ? complete : incomplete; // checking if file has been completed
        printf("%d. %.20s, %s : %s\n", i, head->bpkg->ident, head->bpkg->filename, temp);
        i++;
    }
}