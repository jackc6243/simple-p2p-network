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
};
extern struct bpkg_obj* bpkg;

int initiate_packages(int max_size, struct package_list* all_packages);
int remove_package(struct package_list* list, char* ident);
void add_package(struct package_list* list, struct bpkg_obj* bpkg);

#endif