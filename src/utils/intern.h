#pragma once

#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <utils/buffer.h>
#include <utils/misc.h>
#include <utils/test.h>
#include <utils/log.h>

typedef struct intern_entry {
	uint64_t hash;
	char *string;
} intern_entry_t;

buffer_t(intern_entry_t) interned = NULL;

char *intern_find(uint64_t hash) {
	for (size_t i = 0; i < buffer_len(interned); i++)
		if (interned[i].hash == hash)
			return interned[i].string;

	return NULL;
}

char *intern_str(char *str) {
	uint64_t hash = str_hash(str);

	char *entry = intern_find(hash);
	
	if (entry) 
		return entry;
	else {
		char *buf = heap_string(str);
		intern_entry_t entry = { hash, buf };
		buffer_push(interned, entry);
		return buf;
	}
}

char *intern_range(char *start, char *end) {
	uint64_t hash = str_range_hash(start, end);

	char *entry = intern_find(hash);

	if (entry)
		return entry;
	else {
		char *buf = heap_string_range(start, end);
		intern_entry_t entry = { hash, buf };
		buffer_push(interned, entry);
		return buf;
	}
}

// Adds a dummy string to the interner to initialize the stretchy buffer
void intern_init() {
	char *string = heap_string("Hello, World!");
	
	intern_entry_t entry = { str_hash(string), string };
	buffer_push(interned, entry);
}

void intern_free() {
	for (size_t i = 0; i < buffer_len(interned); i++) {
		free(interned[i].string);
	}

	buffer_free(interned);
}

test_result_t intern_test() {
	char *str1 = intern_str("abc");
	char *str2 = intern_str("abc");
	char *str3 = intern_str("def");

	if (str1 != str2)
		return test_fail("Interner failed to intern similar strings");

	if (str1 == str3 or str2 == str3)
		return test_fail("Interner allocated 2 strings to the same location");

	else 
		return test_pass();
}
