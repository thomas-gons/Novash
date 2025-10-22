/*
 * Novash — a minimalist shell implementation
 * Copyright (C) 2025 Thomas Gons
 *
 * This file is licensed under the GNU General Public License v3 or later.
 * See <https://www.gnu.org/licenses/> for details.
 */

#include "collections.h"


void *arr_grow(void *arr, size_t needed, size_t element_size) {
    size_t new_cap = 0;
    arr_header_t *header;

    // Calculate the new capacity
    if (arr) {
        header = arr_header(arr);
        new_cap = header->capacity;
        
        // Double the capacity until the 'needed' number of slots is satisfied
        while (header->count + needed > new_cap) {
            new_cap = (new_cap == 0) ? SBT_VECTOR_BASE_CAP : new_cap * 2;
        }
    } else {
        // Initial allocation (arr is NULL)
        new_cap = (needed == 0) ? SBT_VECTOR_BASE_CAP : needed; // Start with 16 or 'needed'
    }

    // Calculate the new total size to allocate (Header + Data)
    // The total size includes the space for the header struct itself.
    size_t new_total_size = sizeof(arr_header_t) + new_cap * element_size;

    if (arr) {
        // If 'arr' exists, use xrealloc on the existing header address.
        header = xrealloc(arr_header(arr), new_total_size);
    } else {
        // If 'arr' is NULL, use xmalloc for the initial allocation.
        header = xmalloc(new_total_size);
        header->count = 0; 
    }

    // Update capacity value in the new header block
    header->capacity = new_cap;

    // Return the data pointer (which is located immediately after the header struct).
    return (void *)(header + 1);
}


/* --- LINKED LIST --- */

list_t *list_new(void) {
    list_t *list = xmalloc(sizeof(list_t));
    list->head = NULL;
    list->size = 0;
    return list;
}

void list_add_head(list_t *list, void *data) {
    list_node_t *new_node = xmalloc(sizeof(list_node_t));
    new_node->data = data;
    new_node->next = list->head;
    list->head = new_node;
    list->size++;
}

void list_add(list_t *list, void *data) {
    list_node_t *new_node = xmalloc(sizeof(list_node_t));
    new_node->data = data;
    new_node->next = NULL;

    if (!list->head) {
        list->head = new_node;
    } else {
        list_node_t *current = list->head;
        while (current->next != NULL)
            current = current->next;
        current->next = new_node;
    }
    list->size++;
}

void *list_pop_front(list_t *list) {
    if (!list || !list->head) return NULL;
    list_node_t *node_to_remove = list->head;
    void *data = node_to_remove->data;
    list->head = node_to_remove->next;
    free(node_to_remove);
    list->size--;
    return data;
}

void list_del(list_t *list, void *data) {
    if (!list || !list->head) return;
    list_node_t *current = list->head;
    list_node_t *prev = NULL;

    while (current != NULL) {
        if (current->data == data) {
            if (prev == NULL)
                list->head = current->next;
            else
                prev->next = current->next;
            free(current);
            list->size--;
            return;
        }
        prev = current;
        current = current->next;
    }
}

void list_free(list_t *list, void (*free_data)(void *)) {
    if (!list) return;
    list_node_t *current = list->head;
    while (current) {
        list_node_t *next = current->next;
        if (free_data)
            free_data(current->data);
        free(current);
        current = next;
    }
    free(list);
}

/* --- HASHMAP --- */

hashmap_t *hashmap_new(size_t capacity) {
    hashmap_t *map = xmalloc(sizeof(hashmap_t));
    map->buckets = xmalloc(capacity * sizeof(list_t *));
    for (size_t i = 0; i < capacity; i++)
        map->buckets[i] = list_new();
    map->capacity = capacity;
    map->size = 0;
    return map;
}

static unsigned hash(const char *key) {
    unsigned hash = 5381;
    int c;
    while ((c = *key++))
        hash = ((hash << 5) + hash) + (unsigned) c; // djb2
    return hash;
}

bool hashmap_set(hashmap_t *map, const char *key, void *value) {
    if (!map || !key) return false;

    size_t index = hash(key) % map->capacity;
    list_t *bucket = map->buckets[index];

    for (list_node_t *current = bucket->head; current; current = current->next) {
        hashmap_entry_t *entry = current->data;
        if (strcmp(entry->key, key) == 0) {
            if (entry->value) {
                free(entry->value);
            }
            entry->value = value;
            return true;
        }
    }

    hashmap_entry_t *new_entry = xmalloc(sizeof(hashmap_entry_t));
    new_entry->key = xstrdup(key);
    new_entry->value = value;
    
    list_add_head(bucket, new_entry); 
    
    map->size++;
    return true;
}

void *hashmap_get(hashmap_t *map, const char *key) {
    if (!map || !key) return NULL;

    size_t index = hash(key) % map->capacity;
    list_t *bucket = map->buckets[index];

    for (list_node_t *current = bucket->head; current; current = current->next) {
        hashmap_entry_t *entry = current->data;
        if (strcmp(entry->key, key) == 0)
            return entry->value;
    }
    return NULL;
}

/**
 * Frees a single hashmap entry, including the key string and the value data.
 * This is designed to be passed to list_free.
 */
static void hashmap_entry_free(void *data) {
    if (data == NULL) return;

    hashmap_entry_t *entry = (hashmap_entry_t *)data;
    
    free(entry->key);
    free(entry->value);
    free(entry);
}

void hashmap_free(hashmap_t *map) {
    if (!map) return;
    
    for (size_t i = 0; i < map->capacity; i++) {
        list_free(map->buckets[i], hashmap_entry_free); 
    }
    
    free(map->buckets);
    free(map);
}
