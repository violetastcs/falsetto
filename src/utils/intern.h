/*
 *  This file is part of Falsetto.
 *
 *  Falsetto is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Falsetto is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Falsetto.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <stddef.h>
#include <string.h>
#include <stdint.h>

#include <utils/buffer.h>
#include <utils/misc.h>
#include <utils/test.h>
#include <utils/log.h>

// An interned string
typedef struct intern_entry {
	uint64_t hash;
	char *string;
} intern_entry_t;

buffer_t(intern_entry_t) interned = NULL;

// Look for an interned string by it's hash
char *intern_find(uint64_t hash) {
	for (size_t i = 0; i < buffer_len(interned); i++)
		if (interned[i].hash == hash)
			return interned[i].string;

	return NULL;
}

// If a string has already been interned, return a pointer to it, else allocate and intern provided string
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

// intern_str, except dealing with 2 points within a string
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

// Deallocate all interned strings
void intern_free() {
	for (size_t i = 0; i < buffer_len(interned); i++) {
		free(interned[i].string);
	}

	buffer_free(interned);
}

// Make sure the string interner works as anticipated
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
