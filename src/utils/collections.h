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
#include "utils/memory.h"


/*
 * Stretchy Buffer (stb) Implementation
 *
 * This dynamic array pattern, popularized by Sean Barrett (stb libraries),
 * provides a simple, high-performance way to manage dynamically-sized C arrays.
 * * CORE PRINCIPLE: The array metadata (count and capacity) is stored in the 
 * arr_header_t structure located immediately BEFORE the data pointer returned 
 * to the user.
 * * The macros (arr_len, arr_free, arr_push) use pointer arithmetic (arr_header(a))
 * to "rewind" the pointer and access this hidden header, making the array 
 * behave like a simple C pointer (e.g., array[i]).
 * 
 * * USAGE EXAMPLE:
 * 
 * int *my_array = NULL;                        // 1. Start with a NULL pointer
 * arr_push(my_array, 10);                      // 2. The macro handles allocation/resizing and adds the value
 * printf("Length: %zu\n", arr_len(my_array));  // 3. Access elements
 * arr_free(my_array);                          // 4. Free array (also sets my_array to NULL)
 */

typedef struct {
    size_t count;    /**< Number of elements currently used. */
    size_t capacity; /**< Total allocated capacity. */
    // Array data starts immediately after this structure.
} arr_header_t;

#define SBT_VECTOR_BASE_CAP 16

/** Calculates the header address by offsetting the data pointer address. */
#define arr_header(a) ((arr_header_t *)((char *)(a) - sizeof(arr_header_t)))

/** Returns the number of elements in the array (length). */
#define arr_len(a) ((a) ? arr_header(a)->count : 0)

/** Returns the total allocated capacity of the array. */
#define arr_cap(a) ((a) ? arr_header(a)->capacity : 0)

/** Frees the allocated memory (including the hidden header). */
#define arr_free(a) ((void)((a) ? (free(arr_header(a)), (a) = NULL) : 0))

/**
 * Growth function
 * Handles reallocating memory for the header and data.
 */
void *arr_grow(void *arr, size_t needed, size_t element_size);

/** Checks capacity and expands the array if necessary. */
#define arr_maybegrow(a, n)                                                 \
    ((!(a) || arr_header(a)->count + (n) > arr_header(a)->capacity)         \
     ? ((a) = arr_grow(a, (n), sizeof(*(a))), 0)                            \
     : 0)

/** Appends a new element to the end of the array. (The shortest call site!) */
#define arr_push(a, v)                                                       \
    (arr_maybegrow(a, 1), (a)[arr_len(a)] = (v), arr_header(a)->count++)

/** Appends a new element but does not increase the count. */
#define arr_push_nocount(a, v)                                              \
    (arr_maybegrow(a, 1), (a)[arr_len(a)] = (v))

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

void list_add_head(list_t *list, void *data);

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
 * If NULL, only internal structures are freed.
 */
void hashmap_free(hashmap_t *map);


#endif // __COLLECTIONS_H__
