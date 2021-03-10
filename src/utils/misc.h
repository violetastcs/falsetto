#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>

#include <utils/test.h>
#include <utils/log.h>

char *heap_string(char *string) {
	size_t len = strlen(string);
	char *buf = malloc(len + 1);

	if (buf != NULL) {
		memcpy(buf, string, len);
		buf[len] = 0;
	}

	return buf;
}

char *heap_string_range(char *start, char *end) {
	size_t len = (end - start);
	char *buf = malloc(len + 1);

	if (buf != NULL) {
		memcpy(buf, start, len);
		buf[len] = 0;
	} 

	return buf;
}

// FNV-1 hash
uint64_t buffer_hash(char *buf, size_t len) {
	uint64_t hash = 14695981039346656037U;

	char c;
	for (size_t i = 0; i < len; i++) {
		c = buf[i];
		hash *= 1099511628211;
		hash ^= c;
	}

	return hash;
}

uint64_t str_hash(char *str) {
	return buffer_hash(str, strlen(str));
}

uint64_t str_range_hash(char *start, char *end) {
	return buffer_hash(start, end - start);
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

char *vheap_fmt(char *fmt, va_list args) {
	int size = vsnprintf(NULL, 0, fmt, args) + 1;
	char *buf = malloc(size);
	vsprintf(buf, fmt, args);
	return buf;
}

char *heap_fmt(char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	char *buf = vheap_fmt(fmt, args);
	
	va_end(args);
	return buf;
}

void error(int code, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);

	log_error("%s", vheap_fmt(fmt, args));

	va_end(args);
	exit(code);
}

#define TODO(...) error(1, "TODO: " __VA_ARGS__)
