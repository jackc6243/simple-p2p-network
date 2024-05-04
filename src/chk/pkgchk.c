#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>

#include <sys/stat.h>
#include "../tree/merkletree.h"

#define MAX_LINE_LEN 2048
#define MAX_IDENT 1024
#define MAX_FILENAME 256
#define HASH_SIZE 64

#define TRUE 1
#define FALSE 0

// PART 1
struct bpkg_obj {
    char* ident;
    char* filename;
    uint32_t size;
    struct merkle_tree* tree;
};

void free_array(struct merkle_tree_node** arr, int l) {
    for (int i = 0; i < l; i++) {
        if (arr[i] != NULL) {
            free(arr[i]);
        }
    }
    free(arr);
}

int parse_info(char* buffer, FILE* file, char* name, char delim, void** address, int is_string, int l) {
    char* tok;
    if (fgets(buffer, MAX_LINE_LEN, file)) {
        tok = strtok(buffer, delim);
        // making sure the format is correct
        if (strcmp(tok, name)) {
            return FALSE;
        }

        if (address == NULL && tok != NULL) {
            // When this is no expected data but data has been received
            return FALSE;
        } else if (is_string) {
            // When data is expected to be string
            tok = strtok(NULL, '\n');
            char* data = (char*)malloc(sizeof(l));
            memcpy(data, tok, l);
            *address = data;
        } else {
            // when data is expected to be int
            int num;
            if (sscanf(tok, "%d", &num) != 1) {
                return FALSE;
            }
            *address = num;
        }
    } else { return FALSE; }

    return TRUE;
}

int parse_hashes(char* buffer, FILE* file, struct merkle_tree_node** all_nodes, int start, int end, int isChunk) {

    for (int i = start; i < end; i++) {
        if (fgets(buffer, MAX_LINE_LEN, file) && buffer[0] == '\t') {
            // what to do with each hash
            all_nodes[i] = create_tree_node();
            if (isChunk) {
                all_nodes[i]->is_leaf = TRUE;
            }
            memcpy(all_nodes[i]->expected_hash, buffer + 1, HASH_SIZE);
        } else {
            // If one of hashes not in format, then we have failed
            return FALSE;
        }
    }
    return TRUE;
}

/**
 * Loads the package for when a valid path is given
 */
struct bpkg_obj* bpkg_load(const char* path) {

    // check if path is valid
    struct stat st;
    if (stat(path, &st) < 0) {
        return NULL;
    }

    struct bpkg_obj* obj = (struct bpkg_obj*)malloc(sizeof(struct bpkg_obj));

    FILE* file = fopen(path, "r");
    char* buffer[MAX_LINE_LEN] = { 0 };
    char* tok;

    // Parsing infos
    int nhash, size;
    if (!parse_info(buffer, file, "ident", ':', &(obj->ident), TRUE, MAX_IDENT) ||
        !parse_info(buffer, file, "filename", ':', &(obj->filename), TRUE, MAX_FILENAME) ||
        !parse_info(buffer, file, "size", ':', &size, FALSE, 0) ||
        !parse_info(buffer, file, "nhash", ':', &nhash, FALSE, 0) ||
        !parse_info(buffer, file, "hashes", ':', NULL, TRUE, 0)) {
        free(obj);
        return NULL;
    }

    int height;
    // Checking size is valid


    // Checking nhashes is valid



    // Parsing Hashes
    struct merkle_tree_node** all_nodes = (struct merkle_tree_node**)malloc(sizeof(struct merkle_tree_node*) * nhash);
    if (!parse_hashes(buffer, file, all_nodes, 0, nhash, FALSE)) {
        free_array(all_nodes, nhash);
        free(obj);
        return NULL;
    }

    // Parsing nchunks and checking its validity
    int nchunks;

    // Parsing chunks
    if (!parse_hashes(buffer, file, all_nodes, nhash, nhash + nchunks, TRUE)) {
        free_array(all_nodes, nhash);
        free(obj);
        return NULL;
    }

    obj->tree = level_order_create_tree(all_nodes, nhash + nchunks, height);
    free(all_nodes); // This does not free the nodes inside the array, just the array itself

    return obj;
}

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
struct bpkg_query bpkg_file_check(struct bpkg_obj* bpkg);

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };

    return qry;
}

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_completed_chunks(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    return qry;
}


/**
 * Gets the mininum of hashes to represented the current completion state
 * Example: If chunks representing start to mid have been completed but
 * 	mid to end have not been, then we will have (N_CHUNKS/2) + 1 hashes
 * 	outputted
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query qry = { 0 };
    return qry;
}


/**
 * Retrieves all chunk hashes given a certain an ancestor hash (or itself)
 * Example: If the root hash was given, all chunk hashes will be outputted
 * 	If the root's left child hash was given, all chunks corresponding to
 * 	the first half of the file will be outputted
 * 	If the root's right child hash was given, all chunks corresponding to
 * 	the second half of the file will be outputted
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg,
    char* hash) {

    struct bpkg_query qry = { 0 };
    return qry;
}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    //TODO: Deallocate here!

}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    //TODO: Deallocate here!

}


