#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "../../include/chk/pkgchk.h"
#include "../../include/net/state.h"
#include <pthread.h>


int initiate_packages(struct package_list* all_packages) {
    all_packages->head = NULL;
    all_packages->length = 0;
}

// Add a new package to the end of the list
void add_package(struct package_list* list, struct bpkg_obj* bpkg) {
    struct package* new_package = (struct package*)malloc(sizeof(struct package));
    new_package->bpkg = bpkg;
    new_package->is_completed = 0;
    new_package->next = NULL;

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
    int i = 0;
    char complete[] = "COMPLETED";
    char incomplete[] = "INCOMPLETE";

    char* temp;
    while (head != NULL) {
        temp = head->is_complete ? complete : incomplete; // checking if file has been completed
        printf("%d. %.20s, %s : %s\n", i, head->bpkg->ident, head->bpkg->filename, temp);
        i++;
    }
}