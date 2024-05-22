#ifndef PACKAGE_H
#define PACKAGE_H

#include <stdlib.h>
#include <pthread.h>

struct package_list {
    struct package* head;
    struct package* tail;
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

#endif