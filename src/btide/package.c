#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

#include "../../include/chk/pkgchk.h"
#include "../../include/net/package.h"
#include <pthread.h>


#define TRUE 1
#define FALSE 0

// initate packages
struct package_list* initiate_packages() {
    struct package_list* all_packages = (struct package_list*)malloc(sizeof(struct package_list));
    all_packages->head = NULL;
    all_packages->tail = NULL;
    all_packages->length = 0;

    if (pthread_mutex_init(&(all_packages->plist_lock), NULL) != 0) {
        puts("pthread_mutex_init for package list failed");
        exit(EXIT_FAILURE);
    }

    return all_packages;
}

// Add a new package to the end of the list
int add_package(struct package_list* list, struct bpkg_obj* bpkg, int is_complete) {
    struct package* new_package = (struct package*)malloc(sizeof(struct package));
    new_package->bpkg = bpkg;
    new_package->is_complete = is_complete;
    new_package->next = NULL;
    new_package->previous = list->tail;
    new_package->package_list = list;

    if (pthread_mutex_init(&(new_package->p_lock), NULL) != 0) {
        puts("pthread_mutex_init for package failed");
        free(new_package);
        return FALSE;
    }

    // adding to list, we need to lock it
    pthread_mutex_lock(&(list->plist_lock));
    if (list->head == NULL) {
        list->head = new_package; // list is empty, need to set head
    } else {
        list->tail->next = new_package; // add to the end otherwise
    }
    list->tail = new_package;
    list->length++;
    pthread_mutex_unlock(&(list->plist_lock));

    return TRUE;
}

// finds the rquired package
struct package* get_package(struct package_list* list, char* ident) {
    pthread_mutex_lock(&(list->plist_lock));
    struct package* current = list->head;

    while (current != NULL) {
        if (strncmp(current->bpkg->ident, ident, 20) == 0) {
            pthread_mutex_unlock(&(list->plist_lock));
            return current;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&(list->plist_lock));
    return NULL;
}

// directly removes a package
int remove_package_direct(struct package* package) {
    struct package_list* list = package->package_list;
    pthread_mutex_lock(&((list->plist_lock)));

    // edge case where this package is the head
    if (list->head == package) {
        list->head = package->next;
        if (list->head != NULL) { // check if list is not empty after removal
            list->head->previous = NULL;
        }
    } else {
        package->previous->next = package->next;
        if (package->next != NULL) { // check if package is not the last node
            package->next->previous = package->previous;
        }
    }

    // case where package is the tail
    if (package == list->tail) {
        list->tail = package->previous;
        if (list->tail != NULL) { // check if list is not empty after removal
            list->tail->next = NULL;
        }
    }

    package_destroy(package); // free the package
    list->length--;
    pthread_mutex_unlock(&((list->plist_lock)));
    return 1;
}

// Remove a package from the list, return true if we succesful found one and remove it
int remove_package(struct package_list* list, char* ident) {
    pthread_mutex_lock(&(list->plist_lock));
    struct package* current = list->head;

    while (current != NULL) {
        if (strncmp(current->bpkg->ident, ident, 20) == 0) {
            // we found the matching package, need to lock it since we are changing it
            pthread_mutex_lock(&current->p_lock);
            if (current->previous == NULL) {
                // package is head
                list->head = current->next;
                current->next->previous = NULL;
            } else {
                current->previous->next = current->next;
            }

            // package is tail
            if (current == list->tail) {
                list->tail = current->previous;
            } else {
                current->next->previous = current->previous;
            }

            package_destroy(current);
            list->length--;
            pthread_mutex_unlock(&current->p_lock);
            pthread_mutex_unlock(&(list->plist_lock));
            return TRUE;
        }

        current = current->next;
    }
    pthread_mutex_unlock(&(list->plist_lock));
    return FALSE;
}

// print all packages
void print_packages(struct package_list* list) {
    struct package* current = list->head;  // start at the head of the list
    int i = 1;
    const char* temp;

    while (current != NULL) {
        temp = current->is_complete ? "complete" : "incomplete";  // checking if package has been completed
        printf("%d. %.32s, %s : %s\n", i, current->bpkg->ident, current->bpkg->filename, temp);
        current = current->next;  // move to the next package
        i++;
    }
}

void package_destroy(struct package* package) {
    bpkg_obj_destroy(package->bpkg);
    pthread_mutex_destroy(&package->p_lock); // destroy the mutex
    free(package);
}

// package list resouces
void packagelist_destroy(struct package_list* all_packages) {
    return;
}