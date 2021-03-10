#pragma once

#include <iso646.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <wctype.h>
#include <wchar.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <utils/log.h>
#include <utils/misc.h>
#include <utils/intern.h>
#include <utils/utf8.h>

char *lexer_stream = NULL;
size_t lexer_index = 0;

wint_t peek = WEOF;

// Read a single Unicode codepoint 
wint_t lexer_read() {
	if (lexer_stream[lexer_index] == 0) 
		return WEOF;
	else {
		uint8_t char_size = utf8_char_size(lexer_stream + lexer_index);
		wint_t c = utf8_to_int(lexer_stream + lexer_index);
		lexer_index += char_size;
		wint_t p = peek;
		peek = c;
		return p;
	}
}

// TODO: support Windows/ReactOS
// Open file, initialize the lexer and advance once to initialize peek
void lexer_init_file(char *path) {
	int file = open(path, O_RDONLY);

	if (file == -1) {
		int err = errno;
		error(err, "Failed to open file '%s': %s", path, strerror(err));
	}

	int length = lseek(file, 0, SEEK_END);
	lexer_stream = mmap(0, length, PROT_READ, MAP_PRIVATE, file, 0);

	if (lexer_stream == NULL) {
		int err = errno;
		error(err, "Failed to read file: %s", strerror(err));
	}
	// Initialize peek
	lexer_read();
}

typedef enum token_kind {
	TOKEN_LPAREN,
	TOKEN_RPAREN,
	TOKEN_STRING,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_SYMBOL,
	TOKEN_SPACE,
	TOKEN_EOF
} token_kind_t;

typedef struct token {
	token_kind_t kind;

	size_t start;
	size_t end;

	union {
		char *string_val;
		int64_t int_val;
		double float_val;
		char *symbol_val;
	}
} token_t;

// Parse a single token and advance
token_t lexer_next() {
	token_t token;
	token.start = lexer_index - 1;

	wint_t c = lexer_read();

	switch (c) {
		// End of file
		case WEOF:
			token.kind = TOKEN_EOF;
			break;
		// Open parenthesis
		case '(':
		case '[':
		case '{':
			token.kind = TOKEN_LPAREN;
			break;
		// Close parenthesis
		case ')':
		case ']':
		case '}':
			token.kind = TOKEN_RPAREN;
			break;
		// String literal
		case '"':
			token.kind = TOKEN_STRING;

			while (peek != '"') {
				if (peek == '\\')
					lexer_read();
				lexer_read();
			}

			token.string_val = intern_range(lexer_stream + token.start, lexer_stream + lexer_index);
			
			lexer_read();
			
			break;
		// Integer
		// TODO: parse floating point numbers
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			token.int_val = c - '0';
			while (isdigit(peek)) {
				token.int_val *= 10;
				token.int_val += peek - '0';
				c = lexer_read();
			}

			token.kind = TOKEN_INT;
			break;
		// A symbol (any other valid UTF-8 character)
		default:
			if (utf8_is_whitespace(c)) {
				token.kind = TOKEN_SPACE;
				while (utf8_is_whitespace(peek))
					lexer_read();
			} else {
				token.kind = TOKEN_SYMBOL;
				while (
					!utf8_is_whitespace(peek)
					and peek != '(' 
					and peek != '['
					and peek != '{'
					and peek != ')'
					and peek != ']'
					and peek != '}'
					and peek != '"'
				) {
					lexer_read();
				}

				token.symbol_val = intern_range(lexer_stream + token.start, lexer_stream + lexer_index - 1);
			}
			break;
	}
	
	token.end = lexer_index - 1;

	return token;
}
