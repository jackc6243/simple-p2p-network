#ifndef MERKLETREE_H
#define MERKLETREE_H

#include <stdlib.h>
#include <stddef.h>

struct merkle_tree_node {
    int offset;
    int chunk_size;
    struct merkle_tree_node* left;
    struct merkle_tree_node* right;
    int is_leaf;
    char expected_hash[65]; //Refer to SHA256 Hexadecimal size
    char computed_hash[65];
};
struct merkle_tree {
    struct merkle_tree_node* root;
    struct merkle_tree_node** all_nodes;
    size_t n_nodes;
    size_t max_depth;
};

struct merkle_tree_node* create_tree_node();
struct merkle_tree* create_tree();
void destroy_tree_node(struct merkle_tree_node* node, int recursive);
void destroy_tree(struct merkle_tree* tree, int recursive);
struct merkle_tree* level_order_create_tree(struct merkle_tree_node* arr[], int h);
struct merkle_tree_node* find_hash(struct merkle_tree_node* root, char* hash, int is_expected, int depth, int* depth_add);
void get_min_hashes(struct merkle_tree_node* root, char** arr, int* index);
int get_chunk_from_hash(struct merkle_tree_node* root, char** arr, int i);
void compute_all_hashes(struct merkle_tree_node* root);
struct merkle_tree_node* find_chunk(struct merkle_tree_node* root, char* hash, int offset);

#endif