/* tsbintree - a thread-safe binary tree implementation.
 *
 * This header file defines an implementation of a thread-safe binary tree.
 * Note that the tree is left *unbalanced*. This can lead to poor performance
 * depending on the input, but greatly simplifies the locking strategy.
 *
 * Data is stored in a key=value format.
 *
 * Available operations on the tree:
 *
 * 	tsbintree_init(tsbintree *bt);
 * 	tsbintree_add(tsbintree *bt, char *key, void *value);
 * 	tsbintree_delete(tsbintree *bt, char *key);
 * 	tsbintree_lookup(tsbintree *bt, void **value);
 * 	tsbintree_destroy(tsbintree *bt);
 *
 * If compiled with the TSBT_DEBUG constant defined, then the following *non thread-safe*
 * functions are available:
 *
 * 	tsbintree_print(tsbintree *bt);
 * 	tsbintree_to_dot(tsbintree *bt, char *buffer, int size);
 *
 * See definitions above for the complete signature.
 *
 * Author: Renato Mascarenhas Costa
 */

#ifndef _TSBINTREE_H_
#define _TSBINTREE_H_

#ifndef TSBT_DEBUG
#  define NDEBUG
#endif

#include <errno.h>
#include <assert.h>
#include <pthread.h>

#include <string.h>

#ifdef TSBT_DEBUG
# include <stdio.h>
#endif

#define TSBT_MAX_KEY_SIZE (1024)
#define TSBT_DOT_HEADER ("graph TSBT{")
#define TSBT_DOT_HEADER_LEN ((int) strlen(TSBT_DOT_HEADER))
#define TSBT_MAX_DOT_LABEL_SIZE (5)

/* a tree is just a node with left and right pointers.
 * Each node contains a lock that controls access to that node's data */
struct tsbintree {
	char *key;
	void *value;
	pthread_mutex_t lock;
	struct tsbintree *left;
	struct tsbintree *right;
};

typedef struct tsbintree tsbintree;

/* initializes a tsbintree structure. Must be called before any other function in
 * this library.
 *
 * Returns a non-negative value on success or -1 on error. */
int tsbintree_init(tsbintree *bt);

/* adds a new node to the tree. It is an error to reuse a key.
 * Note that the keys and values are *not* copied. Thus, their references must
 * be valid throughout the use of the library functions.
 *
 * Returns a non-negative value on success or -1 on error. */
int tsbintree_add(tsbintree *bt, char *key, void *value);

/* deletes data referenced by key. It is an error to try to delete an inexisting key.
 *
 * Returns a non-negative value on success or -1 on error. */
int tsbintree_delete(tsbintree *bt, char *key);

/* looks up data referenced by the given key. If it is found, then the `value` pointer
 * will point to the associated data block. Note that no memory copying occurs. If the
 * content of the data pointed to by `value` is changed, so is the related content in
 * the tree. However, such changes are not advisable since it may lead to errors in
 * multi-threaded environments.
 *
 * In case there is no data associated with the given key, an error is returned
 * and the buffer is left unchanged.
 *
 * Returns non-negative on success or -1 on error. */
int tsbintree_lookup(tsbintree *bt, char *key, void **value);

/* frees resources taken by a tree with the given root.
 *
 * Returns non-negative on success or -1 on error. */
int tsbintree_destroy(tsbintree *bt);

#ifdef TSBT_DEBUG

/* generates a representation of the tree's current state usig the DOT language.
 * Can be later inspected with programs such as graphviz. The script content
 * will be stored in the given buffer, which size is given as a third argument.
 *
 * NOTE: this is a debug function. It is **not** thread-safe.
 *
 * Returns non-negative on success or -1 on error. */
int tsbintree_to_dot(tsbintree *bt, char *buffer, int size);

int tsbintree_print(tsbintree *bt);
#endif

#endif /* _TSBINTREE_H_ */
