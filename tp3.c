#include "tp3.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static char *my_strdup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *dup = malloc(len);
    if (!dup) return NULL;
    return memcpy(dup, s, len);
}

typedef struct data {
    char *key;
    void *value;
    bool is_deleted;
} data_t;

struct dictionary {
    data_t *entries;
    size_t size;
    size_t capacity;
    destroy_f destroy;
};

#define C1 1
#define C2 1
#define LOAD_NUM 7
#define LOAD_DEN 10

dictionary_t *dictionary_create(destroy_f destroy) {
    dictionary_t *dict = malloc(sizeof(dictionary_t));
    if (!dict) return NULL;
    dict->entries = calloc(16, sizeof(data_t));
    if (!dict->entries) { free(dict); return NULL; }
    dict->size = 0;
    dict->capacity = 16;
    dict->destroy = destroy;
    return dict;
}

size_t hash_function(const char *key, size_t capacity) {
    size_t hash = 0;
    for (size_t i = 0; key[i] != '\0'; i++)
        hash = ((hash << 5) + hash) + (unsigned char)key[i];
    return hash % capacity;
}

bool dictionary_put(dictionary_t *dictionary, const char *key, void *value) {
    if (!dictionary || !key || key[0] == '\0') return false;
    if ((dictionary->size + 1) * LOAD_DEN > dictionary->capacity * LOAD_NUM) {
        size_t new_capacity = dictionary->capacity * 2;
        data_t *new_entries = calloc(new_capacity, sizeof(data_t));
        if (!new_entries) return false;
        for (size_t i = 0; i < dictionary->capacity; i++) {
            if (dictionary->entries[i].key && !dictionary->entries[i].is_deleted) {
                size_t h0 = hash_function(dictionary->entries[i].key, new_capacity);
                size_t j = 0, h = h0;
                while (new_entries[h].key) {
                    j++;
                    h = (h0 + C1*j + C2*j*j) % new_capacity;
                }
                new_entries[h].key = dictionary->entries[i].key;
                new_entries[h].value = dictionary->entries[i].value;
                new_entries[h].is_deleted = false;
            } else if (dictionary->entries[i].key && dictionary->entries[i].is_deleted) {
                free(dictionary->entries[i].key);
                if (dictionary->destroy && dictionary->entries[i].value)
                    dictionary->destroy(dictionary->entries[i].value);
            }
        }
        free(dictionary->entries);
        dictionary->entries = new_entries;
        dictionary->capacity = new_capacity;
    }
    size_t h0 = hash_function(key, dictionary->capacity);
    size_t original_hash = h0, hash = h0;
    size_t i = 0;
    while (dictionary->entries[hash].key) {
        if (strcmp(dictionary->entries[hash].key, key) == 0) {
            if (!dictionary->entries[hash].is_deleted) {
                if (dictionary->destroy && dictionary->entries[hash].value)
                    dictionary->destroy(dictionary->entries[hash].value);
            } else {
                dictionary->entries[hash].is_deleted = false;
                dictionary->size++;
            }
            dictionary->entries[hash].value = value;
            return true;
        }
        i++;
        hash = (original_hash + C1*i + C2*i*i) % dictionary->capacity;
        if (i == dictionary->capacity) return false;
    }
    dictionary->entries[hash].key = my_strdup(key);
    if (!dictionary->entries[hash].key) return false;
    dictionary->entries[hash].value = value;
    dictionary->entries[hash].is_deleted = false;
    dictionary->size++;
    return true;
}

void *dictionary_get(dictionary_t *dictionary, const char *key, bool *err) {
    if (!dictionary || !key || key[0] == '\0') {
        if (err) *err = true;
        return NULL;
    }
    size_t h0 = hash_function(key, dictionary->capacity);
    size_t original_hash = h0, hash = h0;
    size_t i = 0;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted &&
            strcmp(dictionary->entries[hash].key, key) == 0) {
            if (err) *err = false;
            return dictionary->entries[hash].value;
        }
        i++;
        if (i == dictionary->capacity) break;
        hash = (original_hash + C1*i + C2*i*i) % dictionary->capacity;
    }
    if (err) *err = true;
    return NULL;
}

bool dictionary_delete(dictionary_t *dictionary, const char *key) {
    if (!dictionary || !key || key[0] == '\0') return false;
    size_t h0 = hash_function(key, dictionary->capacity);
    size_t original_hash = h0, hash = h0;
    size_t i = 0;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted &&
            strcmp(dictionary->entries[hash].key, key) == 0) {
            if (dictionary->destroy && dictionary->entries[hash].value) {
                dictionary->destroy(dictionary->entries[hash].value);
                dictionary->entries[hash].value = NULL;
            }
            dictionary->entries[hash].is_deleted = true;
            dictionary->size--;
            return true;
        }
        i++;
        if (i == dictionary->capacity) return false;
        hash = (original_hash + C1*i + C2*i*i) % dictionary->capacity;
    }
    return false;
}

void *dictionary_pop(dictionary_t *dictionary, const char *key, bool *err) {
    if (!dictionary || !key || key[0] == '\0') {
        if (err) *err = true;
        return NULL;
    }
    size_t h0 = hash_function(key, dictionary->capacity);
    size_t original_hash = h0, hash = h0;
    size_t i = 0;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted &&
            strcmp(dictionary->entries[hash].key, key) == 0) {
            void *val = dictionary->entries[hash].value;
            dictionary->entries[hash].value = NULL;
            dictionary->entries[hash].is_deleted = true;
            dictionary->size--;
            if (err) *err = false;
            return val;
        }
        i++;
        if (i == dictionary->capacity) break;
        hash = (original_hash + C1*i + C2*i*i) % dictionary->capacity;
    }
    if (err) *err = true;
    return NULL;
}

bool dictionary_contains(dictionary_t *dictionary, const char *key) {
    if (!dictionary || !key || key[0] == '\0') return false;
    size_t h0 = hash_function(key, dictionary->capacity);
    size_t original_hash = h0, hash = h0;
    size_t i = 0;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted &&
            strcmp(dictionary->entries[hash].key, key) == 0)
            return true;
        i++;
        if (i == dictionary->capacity) return false;
        hash = (original_hash + C1*i + C2*i*i) % dictionary->capacity;
    }
    return false;
}

size_t dictionary_size(dictionary_t *dictionary) {
    return dictionary ? dictionary->size : 0;
}

void dictionary_destroy(dictionary_t *dictionary) {
    if (!dictionary) return;
    for (size_t i = 0; i < dictionary->capacity; i++) {
        if (dictionary->entries[i].key) {
            free(dictionary->entries[i].key);
            if (dictionary->destroy && dictionary->entries[i].value)
                dictionary->destroy(dictionary->entries[i].value);
        }
    }
    free(dictionary->entries);
    free(dictionary);
}