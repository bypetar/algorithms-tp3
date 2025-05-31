#include "tp3.h"
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

static char *my_strdup(const char *s) {
    if (s == NULL) return NULL;
    size_t len = strlen(s) + 1;
    char *new = malloc(len);
    if (new == NULL) return NULL;
    return memcpy(new, s, len);
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

dictionary_t *dictionary_create(destroy_f destroy) {
    dictionary_t *dict = malloc(sizeof(dictionary_t));
    if (!dict){
      return NULL;
    }
    dict->entries = calloc(16, sizeof(data_t));
    if (!dict->entries) {
        free(dict);
        return NULL;
    }
    dict->size = 0;
    dict->capacity = 16;
    dict->destroy = destroy;
    return dict;
}

size_t hash_function(const char *key, size_t capacity) {
    size_t hash = 0;
    for (size_t i = 0; key[i] != '\0'; i++) {
        hash = ((hash << 5) + hash) + key[i];
    }
    return hash % capacity;
}

bool dictionary_put(dictionary_t *dictionary, const char *key, void *value) {
    if (!dictionary || !key || key[0] == '\0'){
      return false;
    }
    if (dictionary->size >= dictionary->capacity) {
        size_t new_capacity = dictionary->capacity * 2;
        data_t *new_entries = calloc(new_capacity, sizeof(data_t));
        if (!new_entries){
          return false;
        }
        for (size_t i = 0; i < dictionary->capacity; i++) {
            if (dictionary->entries[i].key && !dictionary->entries[i].is_deleted) {
                size_t new_hash = hash_function(dictionary->entries[i].key, new_capacity);
                while (new_entries[new_hash].key) {
                    new_hash = (new_hash + 1) % new_capacity;
                }
                new_entries[new_hash].key = dictionary->entries[i].key;
                new_entries[new_hash].value = dictionary->entries[i].value;
                new_entries[new_hash].is_deleted = false;
            } else if (dictionary->entries[i].key && dictionary->entries[i].is_deleted) {
                free(dictionary->entries[i].key);
                if (dictionary->destroy && dictionary->entries[i].value) {
                    dictionary->destroy(dictionary->entries[i].value);
                }
            }

        }
        free(dictionary->entries);
        dictionary->entries = new_entries;
        dictionary->capacity = new_capacity;
    }
    size_t hash = hash_function(key, dictionary->capacity);
    size_t original_hash = hash;
    while (dictionary->entries[hash].key) {
        if (strcmp(dictionary->entries[hash].key, key) == 0) {
            if (!dictionary->entries[hash].is_deleted) {
                if (dictionary->destroy && dictionary->entries[hash].value) {
                    dictionary->destroy(dictionary->entries[hash].value);
                }
                dictionary->entries[hash].value = value;
                return true;
            } else {
                if (dictionary->destroy && dictionary->entries[hash].value) {
                    dictionary->destroy(dictionary->entries[hash].value);
                }
                dictionary->entries[hash].value = value;
                dictionary->entries[hash].is_deleted = false;
                dictionary->size++;
                return true;
            }
        }
        hash = (hash + 1) % dictionary->capacity;
        if (hash == original_hash){
          return false;
        }
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
        if (err){
          *err = true;
        }
        return NULL;
    }
    size_t hash = hash_function(key, dictionary->capacity);
    size_t original_hash = hash;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted && strcmp(dictionary->entries[hash].key, key) == 0) {
            *err = false;
            return dictionary->entries[hash].value;
        }
        hash = (hash + 1) % dictionary->capacity;
        if (hash == original_hash){
          break;
        }
    }
    *err = true;
    return NULL;
}

bool dictionary_delete(dictionary_t *dictionary, const char *key) {
    if (!dictionary || !key || key[0] == '\0'){
      return false;
    }
    size_t hash = hash_function(key, dictionary->capacity);
    size_t original_hash = hash;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted && strcmp(dictionary->entries[hash].key, key) == 0) {
            if (dictionary->destroy && dictionary->entries[hash].value) {
                dictionary->destroy(dictionary->entries[hash].value);
            }
            dictionary->entries[hash].value = NULL;
            dictionary->entries[hash].is_deleted = true;
            dictionary->size--;
            return true;
        }
        hash = (hash + 1) % dictionary->capacity;
        if (hash == original_hash){
          return false;
        }
    }
    return false;
}

void *dictionary_pop(dictionary_t *dictionary, const char *key, bool *err) {
    if (!dictionary || !key || key[0] == '\0') {
        if (err){
          *err = true;
        }
        return NULL;
    }
    size_t hash = hash_function(key, dictionary->capacity);
    size_t original_hash = hash;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted && strcmp(dictionary->entries[hash].key, key) == 0) {
            void *value = dictionary->entries[hash].value;
            dictionary->entries[hash].value = NULL;
            dictionary->entries[hash].is_deleted = true;
            dictionary->size--;
            if (err) *err = false;
            return value;
        }
        hash = (hash + 1) % dictionary->capacity;
        if (hash == original_hash){
           break;
        }
    }
    if (err){
      *err = true;
    }
    return NULL;
}

bool dictionary_contains(dictionary_t *dictionary, const char *key) {
    if (!dictionary || !key || key[0] == '\0'){
       return false;
    }
    size_t hash = hash_function(key, dictionary->capacity);
    size_t original_hash = hash;
    while (dictionary->entries[hash].key) {
        if (!dictionary->entries[hash].is_deleted && strcmp(dictionary->entries[hash].key, key) == 0) {
            return true;
        }
        hash = (hash + 1) % dictionary->capacity;
        if (hash == original_hash){ 
        return false;
      }
    }
    return false;
}

size_t dictionary_size(dictionary_t *dictionary) {
    if (!dictionary){
      return 0;
    }
    return dictionary->size;
}

void dictionary_destroy(dictionary_t *dictionary) {
    if (!dictionary){
      return;
    } 
    for (size_t i = 0; i < dictionary->capacity; i++) {
        if (dictionary->entries[i].key) {
            free(dictionary->entries[i].key);
            if (dictionary->destroy && dictionary->entries[i].value) {
                dictionary->destroy(dictionary->entries[i].value);
            }
        }
    }
    free(dictionary->entries);
    free(dictionary);
}