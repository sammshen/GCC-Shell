// a basic hashmap structure and API
#include <stddef.h>

// key: String -> val: int
typedef struct {
    char *key;
    int value;
} KeyValuePair;

typedef struct HashNode {
    KeyValuePair pair;

    // use chaining to handle collisions
    struct HashNode *next;
} HashNode;

// start with a capacity that is x2 larger than intended
// number of pairs and a prime number
typedef struct {
    HashNode **buckets;
    size_t capacity;
} HashMap;

void initHashMap(HashMap *map, size_t capacity);

void hashMapPut(HashMap *map, char *key, int value);

int hashMapGet(HashMap *map, char *key);

void hashMapRemove(HashMap *map, char *key);

void freeHashMap(HashMap *map);
