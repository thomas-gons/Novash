#include "collections.h"

/* --- DYNAMIC ARRAY --- */

dynarray_t *dynarray_new(size_t initial_capacity) {
    dynarray_t *arr = xmalloc(sizeof(dynarray_t));
    arr->capacity = (initial_capacity > 0) ? initial_capacity : 8;
    arr->size = 0;
    arr->data = xmalloc(arr->capacity * sizeof(void *));
    return arr;
}

bool dynarray_push(dynarray_t *arr, void *item) {
    if (!arr) return false;

    if (arr->size == arr->capacity) {
        arr->capacity *= 2;
        arr->data = xrealloc(arr->data, arr->capacity * sizeof(void *));
    }

    arr->data[arr->size++] = item;
    return true;
}

void *dynarray_get(const dynarray_t *arr, size_t index) {
    if (!arr || index >= arr->size) return NULL;
    return arr->data[index];
}


void dynarray_free(dynarray_t *arr) {
    if (!arr) return;
    free(arr->data);
    free(arr);
}


/* --- LINKED LIST --- */

list_t *list_new(void) {
    list_t *list = xmalloc(sizeof(list_t));
    list->head = NULL;
    list->size = 0;
    return list;
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
            entry->value = value;
            return true;
        }
    }

    hashmap_entry_t *new_entry = xmalloc(sizeof(hashmap_entry_t));
    new_entry->key = xstrdup(key);
    new_entry->value = value;

    list_node_t *new_node = xmalloc(sizeof(list_node_t));
    new_node->data = new_entry;
    new_node->next = bucket->head;
    bucket->head = new_node;

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

void hashmap_free(hashmap_t *map, void (*free_data)(void *)) {
    if (!map) return;
    for (size_t i = 0; i < map->capacity; i++)
        list_free(map->buckets[i], free_data);
    free(map->buckets);
    free(map);
}
