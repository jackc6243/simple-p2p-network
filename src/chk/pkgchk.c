#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <math.h>
#include <unistd.h>
#include <string.h>

#include <sys/stat.h>
#include "../../include/tree/merkletree.h"
#include "../../include/chk/pkgchk.h"

#define MAX_LINE_LEN 2048
#define MAX_IDENT 1024
#define MAX_FILENAME 256
#define HASH_SIZE 64
#define CHUNK_SIZE 4096

#define TRUE 1
#define FALSE 0

void free_array(struct merkle_tree_node** arr, int l) {
    for (int i = 0; i < l; i++) {
        if (arr[i] != NULL) {
            free(arr[i]);
        }
    }
    free(arr);
}

/*
Parse a line in the file given using a given buffer.
The expected format of the line is as follow
<name>:<address>
If format is expected, will return 1 (The address can be null.)
The is_string is a boolean that decide how we will interpret the type
of the address and l is the length of the string if string is expected
*/
int parse_info(char* buffer, FILE* file, char* name, char delim, void* address, int is_string, int l) {
    char* tok;
    if (fgets(buffer, MAX_LINE_LEN, file)) {
        tok = strtok(buffer, &delim);
        // making sure the format is correct
        if (strcmp(tok, name)) {
            return FALSE;
        }

        if (address == NULL && tok != NULL) {
            // When there is no expected data but data has been received
            return FALSE;
        } else if (is_string) {
            // When data is expected to be string
            tok = (char*)strtok(NULL, "\n");
            char* data = (char*)malloc(sizeof(l));
            memcpy(data, tok, l);
            *((char**)address) = data;
        } else {
            // when data is expected to be int
            int num;
            if (sscanf(tok, "%d", &num) != 1) {
                return FALSE;
            }
            *((int*)address) = num;
        }
    } else { return FALSE; }

    return TRUE;
}

/*
Parse a lines of hashes in format from given file:
\t<Hash:64 character>
...
Each hash will be converted to a tree node and stored in the given array all_nodes
The start and end integer tells us the index to which store in the array.

Will return 1 if succesfully parsed.
*/
int parse_hashes(char* buffer, FILE* file, struct merkle_tree_node** all_nodes, int start, int end, int isChunk) {

    for (int i = start; i < end; i++) {
        if (fgets(buffer, MAX_LINE_LEN, file) && buffer[0] == '\t') {
            // For each hash, create a tree node
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
        printf("Path invalid\n");
        return NULL;
    }

    struct bpkg_obj* obj = (struct bpkg_obj*)malloc(sizeof(struct bpkg_obj));

    FILE* file = fopen(path, "r");
    char buffer[MAX_LINE_LEN] = { 0 };

    // Parsing infos
    int nhash, size;
    if (!parse_info(buffer, file, "ident", ':', (void*)&(obj->ident), TRUE, MAX_IDENT) ||
        !parse_info(buffer, file, "filename", ':', (void*)&(obj->filename), TRUE, MAX_FILENAME) ||
        !parse_info(buffer, file, "size", ':', (void*)&size, FALSE, 0) ||
        !parse_info(buffer, file, "nhash", ':', (void*)&nhash, FALSE, 0) ||
        !parse_info(buffer, file, "hashes", ':', NULL, TRUE, 0)) {
        free(obj);
        return NULL;
    }

    // Checking nhash is valid, it must be in the form 2^(h-1) - 1
    float temp = log2(nhash + 1);
    if (ceil(temp) != floor(temp)) {
        // nhash invalid
        free(obj);
        return NULL;
    }
    int height = (int)temp + 1;
    obj->nhash = nhash;

    // Parsing Hashes
    struct merkle_tree_node** all_nodes = (struct merkle_tree_node**)malloc(sizeof(struct merkle_tree_node*) * nhash);
    if (!parse_hashes(buffer, file, all_nodes, 0, nhash, FALSE)) {
        free_array(all_nodes, nhash);
        free(obj);
        return NULL;
    }

    // Parsing nchunk
    int nchunk;
    if (!parse_info(buffer, file, "nchunk", ':', &nchunk, FALSE, 0) ||
        !parse_info(buffer, file, "chunks", ':', NULL, TRUE, 0)) {
        free(obj);
        return NULL;
    }

    // Checking nchunk is valid, it must be in the form 2^(h-1) and it must be the same h as nhash
    temp = log2(nchunk);
    if (ceil(temp) != floor(temp) || (int)temp != height - 1) {
        // nchunk invalid
        free(obj);
        return NULL;
    }
    obj->nchunk = nchunk;

    // Parsing chunks
    if (!parse_hashes(buffer, file, all_nodes, nhash, nhash + nchunk, TRUE)) {
        // if fail to parse all required rows, free memory
        free_array(all_nodes, nhash + nchunk);
        free(obj);
        return NULL;
    }

    obj->tree = level_order_create_tree(all_nodes, height); // create the merkle tree

    return obj;
}

struct bpkg_query* initiate_query(int size) {
    struct bpkg_query* query = (struct bpkg_query*)malloc(sizeof(struct bpkg_query));
    query->hashes = (char**)malloc(sizeof(char*) * size);
    query->len = size;

    return query;
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
struct bpkg_query* bpkg_file_check(struct bpkg_obj* bpkg) {
    struct bpkg_query* query = initiate_query(1);
    // Initiating message
    query->hashes[0] = (char*)calloc(20, sizeof(char));
    FILE* file;

    if (access(bpkg->filename, F_OK) == 0) {
        // file exists
        file = fopen(bpkg->filename, "r");
        char str[] = "File Exists";
        memcpy(query->hashes[0], str, sizeof(str));
    } else {
        // file doesn't exist, will creating file instead
        file = fopen(bpkg->filename, "w");
        // Extending file size
        ftruncate(fileno(file), (bpkg->nchunk) * CHUNK_SIZE * 2 - 1);
        char str[] = "File Created";
        memcpy(query->hashes[0], str, sizeof(str));
    }
    fclose(file);
    return query;
};

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query* bpkg_get_all_hashes(struct bpkg_obj* bpkg) {
    struct bpkg_query* query = initiate_query(bpkg->nchunk + bpkg->nhash);
    for (int i = 0; i < bpkg->nchunk + bpkg->nhash; i++) {
        // looping through all hashes in bpkg object.
        query->hashes[i] = bpkg->all_nodes[i]->computed_hash;
    }

    return query;
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
struct bpkg_query* bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg,
    char* hash) {
    struct bpkg_query* query;

    int height = bpkg->tree->height;
    struct merkle_tree* node = find_hash(bpkg->tree->root, hash, 1, &height);
    if (node != NULL) {
        // found the node with the same hash string
        query = initiate_query((int)pow(2, height + 1) - 1);
        store_hash(node, query->hashes, 0);
    } else {
        query = initiate_query(0);
    }
    return query;
}

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query* bpkg_get_completed_chunks(struct bpkg_obj* bpkg) {
    struct bpkg_query* query = initiate_query(bpkg->nhash);
    FILE* file = fopen(bpkg->filename, "r");

    int i;
    for (i = 0; i < bpkg->nhash; i++) {
        if (!sha256_file_hash(file, CHUNK_SIZE, query->hashes[i])) {
            // File incomplete
            break;
        }
    }

    return query;
}


/**
 * Gets the mininum of hashes to represented the current completion state
 * Example: If chunks representing start to mid have been completed but
 * 	mid to end have not been, then we will have (N_CHUNKS/2) + 1 hashes
 * 	outputted
 *
 * Gets only the required/min hashes to represent the current completion state
 * Return the smallest set of hashes of completed branches to represent the
 * completion state of the file.
 *
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query* bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg) {
    // initiate query
    struct bpkg_query* query = initiate_query((bpkg->nchunk / 2) + 1);

    int index = 0;
    store_min_hash(bpkg->tree->root, query->hashes, &index);

    // index is now the last index
    query->len = index + 1;
    // remove unnecessary length
    query->hashes = realloc(query->hashes, sizeof(char*) * query->len);
    return query;
}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    for (int i = 0; i < qry->len; i++) {
        free(qry->hashes[i]);
        qry->hashes[i] = NULL;
    }
    free(qry->hashes);
    free(qry);
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    free(obj->ident);
    free(obj->filename);
    free(obj->all_nodes);
    destroy_tree(obj->tree, TRUE);
    obj->ident = NULL;
    obj->filename = NULL;
    obj->tree = NULL;
    obj->size = 0;
}


