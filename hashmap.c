#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "hashmap.h"

void initHashMap(HashMap *map, size_t capacity) {
    map->capacity = capacity;
    map->buckets = malloc(capacity * sizeof(HashNode *));
    for (size_t i = 0; i < capacity; i++) {
        map->buckets[i] = NULL;
    }
}

size_t hash(char *key, size_t capacity) {
    size_t hashVal = 0;
    while (*key != '\0') {
        hashVal = *key + 31 * hashVal;
        key++;
    }
    return hashVal % capacity;
}

void hashMapPut(HashMap *map, char *key, int value) {
    size_t index = hash(key, map->capacity);
    HashNode *newNode = malloc(sizeof(HashNode));
    newNode->pair.key = strdup(key);
    newNode->pair.value = value;
    // place newNode at the top of the chain
    newNode->next = map->buckets[index];
    map->buckets[index] = newNode;
}

int hashMapGet(HashMap *map, char *key) {
    size_t index = hash(key, map->capacity);
    HashNode *node = map->buckets[index];
    while (node) {
        if (strcmp(node->pair.key, key) == 0) {
            return node->pair.value;
        }
        node = node->next;
    }
    return -1; // key not found
}

void hashMapRemove(HashMap *map, char *key) {
    size_t index = hash(key, map->capacity);
    HashNode *node = map->buckets[index];
    HashNode *prev = NULL;
    while (node) {
        if (strcmp(node->pair.key, key) == 0) {
            if (prev) { prev->next = node->next; } 
            else { map->buckets[index] = node->next; }
            free(node->pair.key);
            // node->pair.value does not need to be freed
            free(node);
        }
        prev = node;
        node = node->next;
    }
}

void freeHashMap(HashMap *map) {
    for (size_t i = 0; i < map->capacity; i++) {
        HashNode *node = map->buckets[i];
        while (node) {
            HashNode *tmp = node;
            node = node->next;
            free(tmp->pair.key);
            free(tmp);
        }
    }
    free(map->buckets);
}