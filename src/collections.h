/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#ifndef __COLLECTIONS_H__
#define __COLLECTIONS_H__

#include <stddef.h>
#include <stdbool.h>
#include "utils.h"


/* ========================================================================== */
/*                              Dynamic Array API                             */
/* ========================================================================== */

/**
 * @brief Dynamic array (resizable array) storing generic pointers.
 *
 * Implements an automatically resizing array that doubles its capacity when full.
 * Provides fast amortized O(1) append operations and indexed access.
 *
 * @note This structure only manages the container; it does not free the elements
 * stored inside. Use a loop or custom function to free individual items if needed.
 */
typedef struct {
    void **data;      /**< Pointer to array of element pointers. */
    size_t size;      /**< Number of elements currently stored. */
    size_t capacity;  /**< Allocated capacity of the array. */
} dynarray_t;

/**
 * @brief Creates a new dynamic array with a given initial capacity.
 * @param initial_cap Initial number of elements to allocate space for.
 * If zero, a default small capacity is used.
 * @return Pointer to a newly allocated dynamic array.
 */
dynarray_t *dynarray_new(size_t initial_cap);

/**
 * @brief Appends an element to the dynamic array.
 * Automatically grows the internal storage if needed (doubling strategy).
 * @param arr Pointer to the dynamic array.
 * @param item Pointer to the element to append.
 * @return true if the element was successfully added, false otherwise.
 */
bool dynarray_push(dynarray_t *arr, void *item);

/**
 * @brief Retrieves an element by index.
 * @param arr Pointer to the dynamic array.
 * @param index Index of the element to retrieve.
 * @return Pointer to the element at the specified index, or NULL if out of bounds.
 */
void *dynarray_get(const dynarray_t *arr, size_t index);

/**
 * @brief Frees the dynamic array structure and its internal data buffer.
 * Does NOT free the individual elements stored in the array.
 * @param arr Pointer to the dynamic array to free.
 */
void dynarray_free(dynarray_t *arr);


/* ========================================================================== */
/*                              Linked List API                               */
/* ========================================================================== */

/**
 * @brief Node of a singly linked list.
 * Each node holds a pointer to user data and a pointer to the next node.
 */
typedef struct list_node_t {
    void *data;                /**< Pointer to the stored data. */
    struct list_node_t *next;  /**< Pointer to the next node in the list. */
} list_node_t;

/**
 * @brief Simple singly linked list structure.
 * The list keeps track of its size and the pointer to the first element.
 */
typedef struct {
    size_t size;               /**< Number of elements currently in the list. */
    list_node_t *head;         /**< Pointer to the first node. */
} list_t;

/**
 * @brief Allocates and initializes an empty list.
 * @return Pointer to a newly allocated list structure.
 */
list_t *list_new(void);

/**
 * @brief Appends an element to the end of the list.
 * @param list Pointer to the list.
 * @param data Pointer to the data to add (not copied).
 */
void list_add(list_t *list, void *data);


void *list_pop_front(list_t *list);

/**
 * @brief Removes the first occurrence of an element matching the given pointer.
 * @param list Pointer to the list.
 * @param data Pointer to the data to remove (comparison by address, not value).
 * @note Does not free the data itself, only the node.
 */
void list_del(list_t *list, void *data);

/**
 * @brief Frees all nodes in the list and optionally their data.
 * @param list Pointer to the list to free.
 * @param free_data Optional function to free each element's data.
 * If NULL, only the nodes are freed.
 */
void list_free(list_t *list, void (*free_data)(void*));


/* ========================================================================== */
/*                               Hash Map API                                 */
/* ========================================================================== */

/**
 * @brief Key-value pair stored in a hash map bucket.
 */
typedef struct {
    char *key;    /**< Null-terminated string key (duplicated internally). */
    void *value;  /**< Pointer to the associated value. */
} hashmap_entry_t;

/**
 * @brief Simple hash map implementation using separate chaining with linked lists.
 * Each bucket is a linked list of key-value pairs.
 */
typedef struct {
    list_t **buckets;  /**< Array of pointers to bucket lists. */
    size_t capacity;   /**< Total number of buckets. */
    size_t size;       /**< Current number of key-value pairs. */
} hashmap_t;

/**
 * @brief Creates a new hash map with the given bucket capacity.
 * @param capacity Number of buckets to allocate.
 * @return Pointer to the newly created hash map.
 */
hashmap_t *hashmap_new(size_t capacity);

/**
 * @brief Inserts or updates a key-value pair in the hash map.
 * @param map Pointer to the hash map.
 * @param key String key (duplicated internally).
 * @param value Pointer to the value to store.
 * @return true if the insertion or update succeeded, false otherwise.
 * @note If the key already exists, its value is replaced.
 */
bool hashmap_set(hashmap_t *map, const char *key, void *value);

/**
 * @brief Retrieves a value associated with a given key.
 * @param map Pointer to the hash map.
 * @param key String key to look up.
 * @return Pointer to the value if found, or NULL otherwise.
 */
void *hashmap_get(hashmap_t *map, const char *key);

/**
 * @brief Frees the hash map and optionally the stored values.
 * @param map Pointer to the hash map to free.
 * @param free_data Optional function to free each stored value.
 * If NULL, only internal structures are freed.
 */
void hashmap_free(hashmap_t *map, void (*free_data)(void *));


#endif // __COLLECTIONS_H__
