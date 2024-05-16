#include "../../include/tree/merkletree.h"
#include "../../include/crypt/sha256.h"
#include <math.h>
#include <string.h>

#define TRUE 1
#define FALSE 0

// Creating tree and intialising default values
struct merkle_tree* create_tree() {
    struct merkle_tree* tree = (struct merkle_tree*)malloc(sizeof(struct merkle_tree));
    tree->root = NULL;
    tree->n_nodes = 0;
    tree->height = 0;
    return tree;
}

// Creating tree node and intialising default values
struct merkle_tree_node* create_tree_node() {
    struct merkle_tree_node* node = (struct merkle_tree_node*)malloc(sizeof(struct merkle_tree_node));
    node->key = NULL;
    node->value = NULL;
    node->left = NULL;
    node->right = NULL;
    node->is_leaf = FALSE;
    return node;
}

// free tree node
void destroy_tree_node(struct merkle_tree_node* node, int recursive) {
    if (recursive) {
        destroy_tree_node(node->left, recursive);
        destroy_tree_node(node->right, recursive);
    }
    free(node);
}

// free tree
void destroy_tree(struct merkle_tree* tree, int recursive) {
    destroy_tree_node(tree->root, recursive);
    free(tree);
}

// create merkle tree from level order traversal
struct merkle_tree* level_order_create_tree(struct merkle_tree_node* arr[], int h) {
    // creating tree
    struct merkle_tree* tree = create_tree();
    tree->root = arr[0];
    tree->height = h;
    tree->all_nodes = arr;
    int low, high, next_low, next_high;
    low = 0;
    high = 1;

    // looping through all the levels in the array given, then connect children appropriately
    for (int level = 0; level < h - 1; level++) {
        // low and high are the indices of the first and last node + 1 in the current level
        // next_low and next_high are the indices of the first and last node + 1 in the next level
        next_low = high;
        next_high += pow(2, level + 1);
        // j is the indiex of the current node in the current level
        int j = low;

        for (int i = next_low; i < next_high; i += 2) {
            // here we loop through all possible nodes in our current level
            arr[j]->left = arr[i];
            arr[j]->right = arr[i + 1];
            j++; // we must increment the current node as well
        }
        low = next_low;
        high = next_high;
    }

    return tree;
}

/*
Returns the node with the hash given to us. Also saves the height of the hash to the address given by height.

*/
struct merkle_tree_node* find_hash(struct merkle_tree_node* root, char* hash, int is_expected, int* height) {
    if (root == NULL) {
        return NULL;
    }

    // Here we found the hash given
    if ((is_expected && strcmp(root->expected_hash, hash) == 0) ||
        (!is_expected && strcmp(root->computed_hash, hash) == 0)) {
        return root;
    }

    // when we go down we have decrease height
    *height -= 1;
    struct merkle_tree_node* a = find_hash(root->left, hash, is_expected, height);
    if (a == NULL) {
        a = find_hash(root->right, hash, is_expected, height);
    }
    return a;
}

void get_min_hashes(struct merkle_tree_node* root, char** arr, int* index) {
    if (root == NULL) {
        return;
    }

    // if root node is complete
    if (strcmp(root->expected_hash, root->computed_hash) == 0) {
        arr[*index] = root->expected_hash;
        *index += 1;
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
        arr[i] = root->expected_hash;
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