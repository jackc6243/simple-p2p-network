#include "merkletree.h"

#define TRUE 1
#define FALSE 0

struct merkle_tree* create_tree() {
    struct merkle_tree* tree = (struct merkle_tree*)malloc(sizeof(struct merkle_tree));
    tree->root = NULL;
    tree->n_nodes = 0;
    return tree;
}

struct merkle_tree_node* create_tree_node() {
    struct merkle_tree_node* node = (struct merkle_tree_node*)malloc(sizeof(struct merkle_tree_node));
    node->key = NULL;
    node->value = NULL;
    node->left = NULL;
    node->right = NULL;
    node->is_leaf = FALSE;
    return node;
}

void destroy_tree_node(struct merkle_tree_node* node, int recursive) {
    if (recursive) {
        destroy_tree_node(node->left, recursive);
        destroy_tree_node(node->right, recursive);
    }
    free(node);
}

void destroy_tree(struct merkle_tree* tree, int recursive) {
    destroy_tree_node(tree->root, recursive);
    free(tree);
}

// create merkle tree from level order traversal
struct merkle_tree* level_order_create_tree(struct merkle_tree_node* arr[], int n, int h) {
    struct merkle_tree* tree = create_tree();
    tree->root = arr[0];
    int low, high, next_low, next_high;
    low = 0;
    high = 1;
    for (int level = 0; level < h; level++) {
        next_low = high;
        next_high += pow(2, level + 1);
        int j = low;
        for (int i = next_low; i < next_high && i < n; i++) {
            arr[j]->left = arr[i];
            i++;
            if (i >= n) {
                break;
            }
            arr[j]->right = arr[i];
            j++;
        }
        low = next_low;
        high = next_high;
    }

    return tree;
}