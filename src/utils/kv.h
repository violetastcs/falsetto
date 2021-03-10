#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <utils/intern.h>
#include <utils/log.h>
#include <utils/buffer.h>
#include <utils/test.h>

typedef struct kv_value {
	uint64_t hash;
	char *value;
} kv_value_t;

// A hashmap of string -> string
typedef struct kv_store {
	buffer_t(kv_value_t) values;
} kv_store_t;

kv_store_t kv_new() {
	return (kv_store_t){ NULL };
}

// Insert a string with a key
void kv_insert(kv_store_t *store, char *key, char *value) {
	uint64_t hash = str_hash(key);

	buffer_push(store->values, (kv_value_t){ hash, intern_str(value) });
}

// Get a string with a key
char *kv_get(kv_store_t *store, char *key) {
	char *value = NULL;
	uint64_t hash = str_hash(key);

	for (size_t i = 0; i < buffer_len(store->values); i++) {
		if (store->values[i].hash == hash) {
			value = store->values[i].value;
			break;
		}
	}

	return value;
}

// Make sure the kv store is working as intended
test_result_t kv_test() {
	kv_store_t store = { NULL };

	kv_insert(&store, "Hello", "World!");

	char *value = kv_get(&store, "Hello");

	if (value == NULL)
		return test_fail("Expected 'World!', found NULL");

	if (strcmp(value, "World!") != 0) 
		return test_fail("Expected 'World!', found '%s'", value);

	return test_pass();
}
