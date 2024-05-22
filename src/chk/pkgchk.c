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
#include "../../include/crypt/sha256.h"

#define TRUE 1
#define FALSE 0

/*
have to redefine strtok_r myself because doesn't work on my machine for some reason, code exactly from source code: https://codebrowser.dev/glibc/glibc/string/strtok_r.c.html
*/
char* strtok_r(char* str, const char* delim, char** saveptr) {
    char* end;

    if (str == NULL) {
        str = *saveptr;
    }

    if (*str == '\0')
    {
        *saveptr = str;
        return NULL;
    }

    // Skip leading delimiters
    str += strspn(str, delim);
    if (*str == '\0') {
        *saveptr = str;
        return NULL;
    }

    // Find the end of the token
    end = str + strcspn(str, delim);
    if (*end == '\0') {
        *saveptr = end;
        return str;
    }

    // Terminate the token and make *saveptr point past it
    *end = '\0';
    *saveptr = end + 1;
    return str;
}

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
int parse_info(char* buffer, FILE* file, char* name, char* delim, void** address, int is_string, int l) {
    char* tok;
    char* context;
    // printf("doing %s\n", name);
    if (fgets(buffer, MAX_LINE_LEN, file)) {
        // printf("buffer: %s\n", buffer);
        tok = strtok_r(buffer, delim, &context);
        // printf("tok1: %s\n", tok);
        // making sure the format is correct
        if (strcmp(tok, name)) {
            return FALSE;
        }
        tok = strtok_r(NULL, delim, &context);
        // when no data is expected
        if (address == NULL) {
            if (tok == NULL) {
                return TRUE;
            } else {
                // When there is no expected data but data has been received
                // puts("failed here");
                return FALSE;
            }
        }
        // printf("tok2: %s,\n", tok);

        if (is_string) {

            // we don't need to parse any string
            if (l <= 0) {
                return TRUE;
            }

            // When data is expected to be string
            char* data = (char*)malloc(sizeof(char) * (l + 1));
            memcpy(data, tok, l);
            data[l] = '\0'; // null terminate the string
            *address = data;
            // *((char**)address) = data;
        } else {
            // when data is expected to be int
            int num = 0;
            if (sscanf(tok, "%d", &num) != 1) {
                return FALSE;
            }
            // printf("num is: %d\n", num);
            // int** x = (int**)address;
            // printf("x: %d\n", **x);
            **((int**)address) = num;
            // printf("num_now: %d,\n", **((int**)address));
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
                char* context;
                // need to add offset and chunk size
                all_nodes[i]->is_leaf = TRUE;
                char* tok = strtok_r(buffer, ",", &context);
                tok = strtok_r(NULL, ",", &context);
                int temp;
                // saving offset
                if (sscanf(tok, "%d", &temp) == 1) {
                    all_nodes[i]->offset = temp;
                } else {
                    return FALSE;
                }

                tok = strtok_r(NULL, ",", &context);
                // saving chunk size
                if (sscanf(tok, "%d", &temp) == 1) {
                    all_nodes[i]->chunk_size = temp;
                } else {
                    return FALSE;
                }
            }
            memcpy(all_nodes[i]->expected_hash, buffer + 1, HASH_SIZE);
        } else {
            // If one of hashes not in format, then we have failed
            return FALSE;
        }
    }
    return TRUE;
}

// This function is for debugging only
void print_hashes_fromarray(struct merkle_tree_node** arr, int l) {
    for (int i = 0; i < l; i++) {
        printf("%d: %.64s\n", i, arr[i]->expected_hash);
    }

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
    int* nhash_ptr = &nhash; int* size_ptr = &size;
    if (!parse_info(buffer, file, "ident", ":\n", (void**)&(obj->ident), TRUE, MAX_IDENT) ||
        !parse_info(buffer, file, "filename", ":\n", (void**)&(obj->filename), TRUE, MAX_FILENAME) ||
        !parse_info(buffer, file, "size", ":\n", (void**)&size_ptr, FALSE, 0) ||
        !parse_info(buffer, file, "nhashes", ":\n", (void**)&nhash_ptr, FALSE, 0) ||
        !parse_info(buffer, file, "hashes", ":\n", NULL, TRUE, 0)) {
        bpkg_obj_destroy(obj);
        return NULL;
    }

    // Checking nhash is valid, it must be in the form 2^(d-1) - 1
    float temp = log2(nhash + 1);
    if (ceil(temp) != floor(temp)) {
        // nhash invalid
        bpkg_obj_destroy(obj);
        return NULL;
    }
    int depth = (int)temp + 1;

    // updating size and nhash
    obj->nhash = nhash;
    obj->size = size;

    // Parsing Hashes
    struct merkle_tree_node** all_nodes = (struct merkle_tree_node**)
        malloc(sizeof(struct merkle_tree_node*) * ((int)pow(2, depth) - 1));
    if (all_nodes == NULL) {
        // malloc failed
        fprintf(stderr, "Malloc failed to create space for all_nodes");
        return NULL;
    }

    if (!parse_hashes(buffer, file, all_nodes, 0, nhash, FALSE)) {
        free_array(all_nodes, nhash);
        bpkg_obj_destroy(obj);
        return NULL;
    }

    // Parsing nchunk and chunk
    int nchunk;
    int* nchunk_ptr = &nchunk;
    if (!parse_info(buffer, file, "nchunks", ":\n", (void**)&nchunk_ptr, FALSE, 0) ||
        !parse_info(buffer, file, "chunks", ":\n", NULL, TRUE, 0)) {
        bpkg_obj_destroy(obj);
        return NULL;
    }

    // Checking nchunk is valid, it must be in the form 2^(d-1) and it must be the same h as nhash
    temp = log2(nchunk);
    if (ceil(temp) != floor(temp) || (int)temp != depth - 1) {
        // nchunk invalid
        bpkg_obj_destroy(obj);
        return NULL;
    }
    obj->nchunk = nchunk;

    // Parsing chunks
    if (!parse_hashes(buffer, file, all_nodes, nhash, nhash + nchunk, TRUE)) {
        // if fail to parse all required rows, free memory
        free_array(all_nodes, nhash + nchunk);
        bpkg_obj_destroy(obj);
        return NULL;
    }

    // print for debug
    // print_hashes_fromarray(all_nodes, obj->nhash + obj->nchunk);

    obj->tree = level_order_create_tree(all_nodes, depth); // create the merkle tree

    // print for debug
    // printf("ident: %.4s, filename: %s, size: %d, nhash: %d, nchunk: %d,\n", obj->ident, obj->filename, obj->size, obj->nhash, obj->nchunk);
    // printf("depth: %d, length: %d, last_i: %d\n", depth, (int)(pow(2, depth) - 1), obj->nhash + obj->nchunk);

    return obj;
}

struct bpkg_query* initiate_query(int size) {
    struct bpkg_query* query = (struct bpkg_query*)malloc(sizeof(struct bpkg_query));

    if (query == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for query.\n");
        return NULL;  // malloc failed
    }

    query->hashes = (char**)malloc(sizeof(char*) * size);
    if (query->hashes == NULL) {
        fprintf(stderr, "Error: Failed to allocate memory for query hash.\n");
        free(query);  // free the previously allocated memory
        return NULL;  // malloc failed
    }

    // initiating string for each hash
    for (int i = 0; i < size; i++) {
        query->hashes[i] = (char*)malloc(65 * sizeof(char));
        // if we fail to malloc memory
        if (query->hashes[i] == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for query hash string.\n");
            // Free previously allocated memory
            for (int j = 0; j < i; j++) {
                free(query->hashes[j]);
            }
            free(query->hashes);
            free(query);
            return NULL;  // malloc failed
        }
        query->hashes[i][64] = '\0';
    }

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
    FILE* file;
    file = fopen(bpkg->filename, "r");

    if (file != NULL) {
        // file exists
        char str[] = "File Exists\0";
        memcpy(query->hashes[0], str, sizeof(str));
    } else {
        // file doesn't exist, will creating file instead
        file = fopen(bpkg->filename, "w");
        // Extending file size
        ftruncate(fileno(file), bpkg->size);
        char str[] = "File Created\0";
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
        memcpy(query->hashes[i], bpkg->tree->all_nodes[i]->expected_hash, 64);
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

    int depth;
    struct merkle_tree_node* node = find_hash(bpkg->tree->root, hash, 1, bpkg->tree->max_depth, &depth);
    // printf("size: %d\n", (int)pow(2, depth - 1));
    if (node != NULL) {
        // found the node with the same hash string
        query = initiate_query((int)pow(2, depth - 1));
        get_chunk_from_hash(node, query->hashes, 0);
    } else {
        query = initiate_query(0);
    }
    return query;
}


struct bpkg_query* bpkg_all_chunks_from_file(struct bpkg_obj* bpkg) {
    struct bpkg_query* query = initiate_query(bpkg->nchunk);
    FILE* file = fopen(bpkg->filename, "r");

    if (file == NULL) {
        perror("Failed to open file");
        return NULL;
    }

    int i;
    for (i = 0; i < bpkg->nchunk; i++) {
        if (!sha256_file_hash(file, (int)(bpkg->size / bpkg->nchunk), query->hashes[i])) {
            // File incomplete
            i++;
            break;
        }
    }

    // updating length in case file was incomplete and we did not compute every nchunk
    query->len = i;

    // remove unnecessary length
    if (query->len < bpkg->nhash) {
        // need to free the excess hashes
        for (int i = query->len; i < bpkg->nhash; i++) {
            free(query->hashes[i]);
        }

        query->hashes = realloc(query->hashes, sizeof(char*) * query->len);
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
    // parse the bpkg data file
    struct bpkg_query* all_chunks = bpkg_all_chunks_from_file(bpkg);
    // update the last level of the tree
    update_computed_chunk_hash(bpkg, all_chunks);
    bpkg_query_destroy(all_chunks);
    struct bpkg_query* query = initiate_query(bpkg->nchunk);
    int idx = 0;

    for (int i = bpkg->nhash; i < bpkg->nhash + bpkg->nchunk; i++) {
        // we only add to the return query if the hash is correct
        if (strncmp(bpkg->tree->all_nodes[i]->computed_hash, bpkg->tree->all_nodes[i]->expected_hash, 64) == 0) {
            memcpy(query->hashes[idx], bpkg->tree->all_nodes[i]->computed_hash, 64);
            idx++;
        }
    }

    // freeing any potentially uneeded strings in query hashes
    for (;idx < bpkg->nchunk; idx++) {
        free(query->hashes[idx]);
    }

    return query;
}

void update_computed_chunk_hash(struct bpkg_obj* bpkg, struct bpkg_query* all_chunks) {
    for (int i = bpkg->nhash; i < bpkg->nhash + bpkg->nchunk; i++) {
        memccpy(bpkg->tree->all_nodes[i]->computed_hash, all_chunks->hashes[i - bpkg->nhash], '\0', 64);
    }
}

// parse all data chunks and update all hashes in the merkle tree
void parse_hash_data_chunks(struct bpkg_obj* bpkg) {
    // parse the bpkg data file
    struct bpkg_query* all_chunks = bpkg_all_chunks_from_file(bpkg);

    // update the last level of the tree
    update_computed_chunk_hash(bpkg, all_chunks);

    // update the hashes in the tree
    compute_all_hashes(bpkg->tree->all_nodes[0]);

    // free the all_chunks query since it is no longer needed
    bpkg_query_destroy(all_chunks);
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
    // parse all data chunks and update hash in tree first
    parse_hash_data_chunks(bpkg);

    // initiate query
    struct bpkg_query* query = initiate_query((bpkg->nchunk / 2) + 1);

    // get min hash
    int index = 0;
    get_min_hashes(bpkg->tree->root, query->hashes, &index);

    // index is the last valid index in query->hashes
    query->len = index;
    // remove unnecessary length and free appropriate hashes
    // puts("freeing");
    for (int i = query->len; i < (bpkg->nchunk / 2) + 1; i++) {
        // printf("%d,", i);
        free(query->hashes[i]);
        query->hashes[i] = NULL;
    }
    // printf("query outside 0: %s\n", query->hashes[0]);
    // free(*query->hashes);
    // reallocate because we don't need extra space
    // query->hashes = realloc(query->hashes, sizeof(char*) * query->len);
    return query;

}


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry) {
    if (qry != NULL) {
        if (qry->hashes != NULL) {
            for (int i = 0; i < qry->len; i++) {
                // make sure we don't accidently double free
                if (qry->hashes[i] != NULL) {
                    free(qry->hashes[i]);
                }
            }
            free(qry->hashes);
        }
        free(qry);
    }
}

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj) {
    if (obj->ident) {
        free(obj->ident);
    }
    if (obj->filename) {
        free(obj->filename);
    }
    destroy_tree(obj->tree, TRUE);
    obj->ident = NULL;
    obj->filename = NULL;
    obj->tree = NULL;
    obj->size = 0;
    free(obj);
}


