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
#include <stdint.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

#include <utils/test.h>
#include <utils/log.h>

void error(int, char*, ...);

// Print a new line and N tabs
void nlt(int n) {
	printf("\n");
	for (; n >= 0; n--)
		// Spaces for tabs to avoid ugly output with larger tab sizes
		printf("    ");
}

// Allocate a string on the heap
char *heap_string(char *string) {
	size_t len = strlen(string);
	char *buf = malloc(len + 1);

	if (buf != NULL) {
		memcpy(buf, string, len);
		buf[len] = 0;
	}

	return buf;
}

// Slice a string between start and end and place on the heap
char *heap_string_range(char *start, char *end) {
	size_t len = (size_t)(end - start);
	char *buf = malloc(len + 1);

	if (buf != NULL) {
		memcpy(buf, start, len);
		buf[len] = 0;
	} 

	return buf;
}

// Get the FNV-1 hash of a buffer
uint64_t buffer_hash(char *buf, size_t len) {
	uint64_t hash = 14695981039346656037U;

	char c;
	for (size_t i = 0; i < len; i++) {
		c = buf[i];
		hash *= 1099511628211;
		hash ^= (uint64_t)c;
	}

	return hash;
}

// Get the FNV-1 hash of a string
uint64_t str_hash(char *str) {
	return buffer_hash(str, strlen(str));
}

// Get the FNV-1 hash of a slice of a string
uint64_t str_range_hash(char *start, char *end) {
	size_t length = (size_t)(end - start);
	return buffer_hash(start, length);
}

test_result_t misc_test() {
	char vals[] = {1, 2, 3, 4};
	char not_vals[] = {5, 6, 7, 8};

	if (buffer_hash(vals, 4) == buffer_hash(not_vals, 4)) 
		return test_fail("Buffer hashes are equal when they should not be");

	char *string = "Hello, World!";
	char *string2 = "Hello, World!abc";

	if (str_hash(string) != str_range_hash(string2, string2 + strlen(string)))
		return test_fail("Buffer hashes should be equal");

	return test_pass();
}

// Heap allocate a formatted string based on a varargs list
char *vheap_fmt(char *fmt, va_list args) {
	int size = vsnprintf(NULL, 0, fmt, args) + 1;

	if (size < 0) {
		int err = errno;
		error(err, "Failed to format string '%s': %s", fmt, strerror(err));
	}
	
	char *buf = malloc((size_t)size);
	vsprintf(buf, fmt, args);
	return buf;
}

// Heap allocate a formatted string based on passed arguments
char *heap_fmt(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char *buf = vheap_fmt(fmt, args);
	
	va_end(args);
	return buf;
}

// Print an error message and exit with code
void error(int code, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	log_error("%s", vheap_fmt(fmt, args));

	va_end(args);
	exit(code);
}

// Exits with a TODO message
#define TODO(...) error(1, "TODO: " __VA_ARGS__)
