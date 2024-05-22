#ifndef PACKAGE_H
#define PACKAGE_H

#include <stdlib.h>
#include <pthread.h>


struct package_list {
    struct package* head;
    struct package* tail;
    pthread_mutex_t plist_lock;
    int length;
};

struct package {
    struct bpkg_obj* bpkg;
    int is_complete;
    struct package* next;
    struct package* previous;
    struct package_list* package_list;
    pthread_mutex_t p_lock;
};
extern struct bpkg_obj* bpkg;

struct package_list* initiate_packages();
int add_package(struct package_list* list, struct bpkg_obj* bpkg, int is_completed);
int remove_package(struct package_list* list, char* ident);
int remove_package_direct(struct package* package);
struct package* get_package(struct package_list* list, char* ident);
void package_destroy(struct package* package);
void print_packages(struct package_list* list);
void packagelist_destroy(struct package_list* all_packages);

#endif