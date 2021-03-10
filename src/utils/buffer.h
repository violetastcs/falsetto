#pragma once

// Stretchy buffers, invented by Sean Barrett

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>
#include <stdlib.h>

#include <utils/log.h>

#define MAX(x, y) ((x) >= (y) ? (x) : (y))

typedef struct buffer_header {
	size_t len;
	size_t cap;
	char buf[];
} buffer_header_t;

#define buffer_t(x) x*

#define buffer__hdr(b) ((buffer_header_t *)((char *)(b) - offsetof(buffer_header_t, buf)))

#define buffer_len(b) ((b) ? buffer__hdr(b)->len : 0)
#define buffer_cap(b) ((b) ? buffer__hdr(b)->cap : 0)
#define buffer_end(b) ((b) + buffer_len(b))

#define buffer_free(b) ((b) ? (free(buffer__hdr(b)), (b) = NULL) : 0)
#define buffer_fit(b, n) ((n) <= buffer_cap(b) ? 0 : ((b) = buffer__grow((b), (n), sizeof(*(b)))))
#define buffer_push(b, ...) (buffer_fit((b), 1 + buffer_len(b)), (b)[buffer__hdr(b)->len++] = (__VA_ARGS__))

void *buffer__grow(const void *buf, size_t new_len, size_t elem_size) {
	assert(buffer_cap(buf) <= (SIZE_MAX - 1)/2);
	size_t new_cap = MAX(16, MAX(1 + 2*buffer_cap(buf), new_len));
	assert(new_len <= new_cap);
	assert(new_cap <= (SIZE_MAX - offsetof(buffer_header_t, buf))/elem_size);
	size_t new_size = offsetof(buffer_header_t, buf) + new_cap*elem_size;
	buffer_header_t *new_hdr;
	if (buf) {
		new_hdr = realloc(buffer__hdr(buf), new_size);
	} else {
		new_hdr = malloc(new_size);
		new_hdr->len = 0;
	}   
	new_hdr->cap = new_cap;
	return new_hdr->buf;
}
