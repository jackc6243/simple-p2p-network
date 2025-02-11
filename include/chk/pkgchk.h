#ifndef PKGCHK_H
#define PKGCHK_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define MAX_LINE_LEN 2048
#define MAX_IDENT 1024
#define MAX_FILENAME 256
#define HASH_SIZE 64

/**
 * Query object, allows you to assign
 * hash strings to it.
 * Typically: malloc N number of strings for hashes
 *    after malloc the space for each string
 *    Make sure you deallocate in the destroy function
 */
struct bpkg_query {
	char** hashes;
	size_t len;
};

//bpkg object that stores all information relating to bpkg file
struct bpkg_obj {
	char* ident;
	char* filename;
	char* full_path;
	uint32_t size;
	uint32_t nhash;
	uint32_t nchunk;
	struct merkle_tree* tree;
};

int parse_info(char* buffer, FILE* file, char* name, char* delim, void** address, int is_string, int l);

// same as the one from c library
char* strtok_r(char* str, const char* delim, char** saveptr);

/**
 * Loads the package for when a value path is given
 */
struct bpkg_obj* bpkg_load(char* directory, char* bpkg_filename);

/**
 * Checks to see if the referenced filename in the bpkg file
 * exists or not.
 * @param bpkg, constructed bpkg object
 * @return query_result, a single string should be
 *      printable in hashes with len sized to 1.
 * 		If the file exists, hashes[0] should contain "File Exists"
 *		If the file does not exist, hashes[0] should contain "File Created"
 */
struct bpkg_query* bpkg_file_check(struct bpkg_obj* bpkg);

/**
 * Retrieves a list of all hashes within the package/tree
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query* bpkg_get_all_hashes(struct bpkg_obj* bpkg);

/**
 * Retrieves all completed chunks of a package object
 * @param bpkg, constructed bpkg object
 * @return query_result, This structure will contain a list of hashes
 * 		and the number of hashes that have been retrieved
 */
struct bpkg_query* bpkg_get_completed_chunks(struct bpkg_obj* bpkg);


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
struct bpkg_query* bpkg_get_min_completed_hashes(struct bpkg_obj* bpkg);


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
struct bpkg_query* bpkg_get_all_chunk_hashes_from_hash(struct bpkg_obj* bpkg, char* hash);


/**
 * Deallocates the query result after it has been constructed from
 * the relevant queries above.
 */
void bpkg_query_destroy(struct bpkg_query* qry);

/**
 * Deallocates memory at the end of the program,
 * make sure it has been completely deallocated
 */
void bpkg_obj_destroy(struct bpkg_obj* obj);

// update the computed_hash of the chunks level of the merklet tree from the given all_chunks
void update_computed_chunk_hash(struct bpkg_obj* bpkg, struct bpkg_query* all_chunks);


struct merkle_tree_node* find_chunk(struct merkle_tree_node* root, char* hash, int offset);

struct bpkg_obj* bpkg_initiate();

struct bpkg_query* bpkg_get_all_hashes_nochunks(struct bpkg_obj* bpkg);

#endif

