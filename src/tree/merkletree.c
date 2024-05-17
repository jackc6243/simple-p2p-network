#include "../../include/tree/merkletree.h"
#include "../../include/crypt/sha256.h"
#include <math.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

// Creating tree and intialising default values
struct merkle_tree* create_tree() {
    struct merkle_tree* tree = (struct merkle_tree*)malloc(sizeof(struct merkle_tree));
    if (tree == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for tree.\n");
        return NULL;
    }

    tree->root = NULL;
    tree->all_nodes = NULL;
    tree->n_nodes = 0;
    tree->max_depth = 0;
    return tree;
}

// Creating tree node and intialising default values
struct merkle_tree_node* create_tree_node() {
    struct merkle_tree_node* node = (struct merkle_tree_node*)malloc(sizeof(struct merkle_tree_node));
    if (node == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for tree node.\n");
        return NULL;
    }

    node->offset = 0;
    node->chunk_size = 0;
    node->left = NULL;
    node->right = NULL;
    node->is_leaf = FALSE;
    node->computed_hash[64] = '\0';
    node->expected_hash[64] = '\0';
    return node;
}

// free tree node
void destroy_tree_node(struct merkle_tree_node* node, int recursive) {
    if (node == NULL) {
        return;
    }

    if (recursive) {
        destroy_tree_node(node->left, recursive);
        destroy_tree_node(node->right, recursive);
    }
    free(node);
}

// free tree
void destroy_tree(struct merkle_tree* tree, int recursive) {
    destroy_tree_node(tree->root, recursive);
    free(tree->all_nodes);
    free(tree);
}

// create merkle tree from level order traversal
struct merkle_tree* level_order_create_tree(struct merkle_tree_node* arr[], int d) {
    // creating tree
    struct merkle_tree* tree = create_tree();
    tree->root = arr[0];
    tree->max_depth = d;
    tree->all_nodes = arr;
    int low, high, next_low, next_high;
    low = 0;
    high = 1;

    // debug stuff
    // printf("depth: %d\n", d);

    // looping through all the levels in the array given, then connect children appropriately
    for (int level = 0; level < d - 1; level++) {
        // low and high are the indices of the first and last node + 1 in the current level
        // next_low and next_high are the indices of the first and last node + 1 in the next level
        next_low = high;
        next_high = next_low + (int)pow(2, level + 1);

        // debug print statement
        // printf("low: %d, high: %d, next_low: %d, next_high: %d\n", low, high, next_low, next_high);

        // prev is the index of the current node in the current level
            // here we loop through all possible nodes in our current level
        int next = next_low;
        for (int prev = low; prev < high; prev++) {
            arr[prev]->left = arr[next];
            arr[prev]->right = arr[next + 1];
            next += 2; // we must increment the next node by 2
        }
        low = next_low;
        high = next_high;
    }

    return tree;
}

/*
Returns the node with the hash given to us. Also saves the depth of the hash to the address given by height.
*/
struct merkle_tree_node* find_hash(struct merkle_tree_node* root, char* hash, int is_expected, int depth, int* depth_add) {
    // printf("hash at: %.5s at subtree_depth: %d,\n", root->expected_hash, depth);

    // Here we found the hash given
    if ((is_expected && strncmp(root->expected_hash, hash, 64) == 0) ||
        (!is_expected && strncmp(root->computed_hash, hash, 64) == 0)) {
        *depth_add = depth;
        return root;
    }

    // did not find the hash
    if (root->is_leaf) {
        return NULL;
    }

    // when we go down we have decrease the subtree_depth
    struct merkle_tree_node* a = find_hash(root->left, hash, is_expected, depth - 1, depth_add);
    if (a == NULL) {
        a = find_hash(root->right, hash, is_expected, depth - 1, depth_add);
    }
    return a;
}

void get_min_hashes(struct merkle_tree_node* root, char** arr, int* index) {
    // if root node is complete
    if (strncmp(root->expected_hash, root->computed_hash, 64) == 0) {
        arr[*index] = root->expected_hash;
        *index += 1;
        return;
    }

    if (root->is_leaf) {
        return;
    }

    // If root node not complete
    get_min_hashes(root->left, arr, index);
    get_min_hashes(root->right, arr, index);
}

/*
Stores all chunks under a hash into given array in subtree of root into array arr
*/
int get_chunk_from_hash(struct merkle_tree_node* root, char** arr, int i) {
    // base case where we reached the chunk
    if (root->left == NULL) {
        memcpy(arr[i], root->expected_hash, 64);
        return i;
    }
    i = get_chunk_from_hash(root->left, arr, i);
    i = get_chunk_from_hash(root->right, arr, i + 1);
    return i;
}

void compute_all_hashes(struct merkle_tree_node* root) {
    // if we are a leaf node, we should already have computed the hash seperately
    // in another function. Thus no computation needed
    if (root->is_leaf) {
        return;
    }

    // Compute hash of left and right child
    compute_all_hashes(root->left);
    compute_all_hashes(root->right);

    // append left hash and right hash together
    char* new_hash = (char*)malloc(sizeof(char) * SHA256_CHUNK_SZ * 2);
    memcpy(new_hash, root->left->computed_hash, SHA256_CHUNK_SZ);
    memcpy(new_hash + SHA256_CHUNK_SZ, root->right->computed_hash, SHA256_CHUNK_SZ);

    // Hash the the 128 size string together and store it in root->computed_hash
    sha256_string_hash((void*)new_hash, (size_t)SHA256_CHUNK_SZ * 2, root->computed_hash);
}