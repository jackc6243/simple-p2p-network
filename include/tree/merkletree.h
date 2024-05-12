#ifndef MERKLETREE_H
#define MERKLETREE_H

#include <stdlib.h>
#include <stddef.h>

struct merkle_tree_node {
    void* key;
    void* value;
    struct merkle_tree_node* left;
    struct merkle_tree_node* right;
    int is_leaf;
    char expected_hash[64]; //Refer to SHA256 Hexadecimal size
    char computed_hash[64];
};
struct merkle_tree {
    struct merkle_tree_node* root;
    struct merkle_tree_node** all_nodes;
    size_t n_nodes;
    size_t height;
};

struct merkle_tree_node* create_tree_node();
struct merkle_tree* create_tree();
void destroy_tree_node(struct merkle_tree_node* node, int recursive);
void destroy_tree(struct merkle_tree* tree, int recursive);
struct merkle_tree* level_order_create_tree(struct merkle_tree_node* arr[], int h);

#endif