#include <stdlib.h>
#include "hashmap.h"

struct hash_map *hash_map_new(size_t (*hash)(void *), int (*cmp)(void *, void *),
                              void (*key_destruct)(void *), void (*value_destruct)(void *)) {
    if (hash == NULL || cmp == NULL || key_destruct == NULL || value_destruct == NULL) {
        return NULL;
    }
    struct hash_map *map = malloc(sizeof(struct hash_map));
    // init map
    map->hash = hash;
    map->cmp = cmp;
    map->key_destruct = key_destruct;
    map->value_destruct = value_destruct;
    map->n_entries = 0;
    map->n_chains = 0;
    map->capacity = 16;
    map->chains = malloc(sizeof(struct chain *) * 16);

    return map;
}

struct chain *new_chain() {
    struct chain *chain = malloc(sizeof(struct chain));
    chain->size = 0;
    chain->capacity = 4;
    chain->entries = malloc(sizeof(struct entry *) * 4);
    return chain;
}

void extend_chain(struct chain *chain) {
    chain->capacity *= 2;
    chain->entries = realloc(chain->entries, chain->capacity);
}

int has_same_entry(int (*cmp)(void *, void *), struct chain *chain, void *k) {
    for (size_t i = 0; i < chain->size; ++i) {
        struct entry *entry = chain->entries[i];
        if (cmp(entry->key, k) == 0) {
            return i;
        }
    }
    return -1;
}

void extend_map(struct hash_map *map) {
    if ((float) map->n_chains / (float) map->capacity < 0.75) {
        return;
    }
    struct entry **buffer = malloc(sizeof(struct entry *) * map->n_entries);
    int index = 0;
    for (size_t i = 0; i < map->capacity; ++i) {
        struct chain *chain = map->chains[i];
        if (chain == NULL){
            continue;
        }
        for (size_t j = 0; j < chain->size; ++j) {
            struct entry *entry = chain->entries[j];
            if (entry != NULL) {
                buffer[index] = entry;
                index++;
            }
        }
        free(chain->entries);
        free(chain);
    }
    free(map->chains);
    map->capacity *= 2;
    map->chains = malloc(sizeof(struct chain*) * map->capacity);
    for (int k = 0; k < index; ++k) {
        struct entry *entry = buffer[k];
        hash_map_put_entry_move(map, entry->key, entry->key);
        free(entry);
    }
    free(buffer);
}

void hash_map_put_entry_move(struct hash_map *map, void *k, void *v) {
    if (map == NULL || k == NULL || v == NULL) {
        return;
    }

    // work out the hash value as index
    size_t index = map->hash(k) % map->capacity;
    struct chain *chain = map->chains[index];

    // check if there's a same key
    int index_existing = has_same_entry(map->cmp, chain, k);
    if (index_existing != -1) {
        // modify the existing one
        map->value_destruct(chain->entries[index_existing]->value);
        chain->entries[index_existing]->value = v;
        return;
    }

    // new an entry
    struct entry *new_entry = malloc(sizeof(struct entry));
    new_entry->key = k;
    new_entry->value = v;
    // new a chain if NULL
    if (map->chains[index] == NULL) {
        map->chains[index] = new_chain();
        map->n_chains++;
    }

    // extend the chain if full
    if (chain->size == chain->capacity) {
        extend_chain(chain);
    }
    // insert the new entry
    chain->entries[chain->size] = new_entry;
    chain->size++;
    map->n_entries++;
    extend_map(map);
}

void hash_map_remove_entry(struct hash_map *map, void *k) {
    if (map == NULL || k == NULL) {
        return;
    }
    // work out the hash value as index
    size_t index = map->hash(k) % map->capacity;
    struct chain *chain = map->chains[index];
    if (chain == NULL) {
        return;
    }
    int index_existing = has_same_entry(map->cmp, chain, k);
    if (index_existing != -1) {
        map->key_destruct(chain->entries[index_existing]->key);
        map->value_destruct(chain->entries[index_existing]->value);
        free(chain->entries[index_existing]);
        chain->entries[index_existing] = NULL;
        map->n_entries--;
    }
}

void *hash_map_get_value_ref(struct hash_map *map, void *k) {
    if (map == NULL || k == NULL) {
        return NULL;
    }
    size_t index = map->hash(k) % map->capacity;
    struct chain *chain = map->chains[index];
    if (chain == NULL) {
        return NULL;
    }

    int index_existing = has_same_entry(map->cmp, chain, k);
    if (index_existing != -1) {
        return chain->entries[index_existing]->value;
    }
    return NULL;
}

void hash_map_destroy(struct hash_map *map) {
    for (size_t i = 0; i < map->n_chains; ++i) {
        struct chain *chain = map->chains[i];
        if (chain == NULL){
            continue;
        }
        for (size_t j = 0; j < chain->size; ++j) {
            struct entry *entry = chain->entries[j];
            if (entry == NULL){
                continue;
            }
            // free key and value
            map->key_destruct(entry->key);
            map->value_destruct(entry->value);
            // free the entry
            free(entry);
        }
        // free entries
        free(chain->entries);
        // free the chain
        free(chain);
    }
    // free chains
    free(map->chains);
    // free the map
    free(map);
}
