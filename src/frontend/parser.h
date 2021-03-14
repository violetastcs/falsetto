#pragma once

#include <stddef.h>
#include <stdint.h>
#include <iso646.h>

#include <utils/intern.h>
#include <utils/buffer.h>
#include <utils/log.h>
#include <utils/misc.h>

#include <frontend/lexer.h>

typedef enum atom_kind {
	ATOM_INTEGER,
	ATOM_FLOAT,
	ATOM_SYMBOL,
	ATOM_STRING,
	ATOM_EXPR
} atom_kind_t;

typedef struct atom {
	atom_kind_t kind;

	union {
		uint64_t integer_val;
		double float_val;
		char *symbol_val;
		char *string_val;
		buffer_t(struct atom) expr;
	}
} atom_t;

// Check if atom is a symbol and, if name != NULL, if it is equal to the name provided
bool is_symbol(atom_t expr, char *name) {
	if (name == NULL)
		return expr.kind == ATOM_SYMBOL;
	else 
		return expr.kind == ATOM_SYMBOL and strcmp(expr.symbol_val, name) == 0;
}

// Print out an atom (for debugging only)
void print_expr(atom_t atom) {
	switch (atom.kind) {
		case ATOM_INTEGER:
			log_trace("Integer (%ld) ", atom.integer_val);
			break;
		case ATOM_FLOAT:
			log_trace("Float (%f) ", atom.float_val);
			break;
		case ATOM_SYMBOL:
			log_trace("Symbol (%s) ", atom.symbol_val);
			break;
		case ATOM_STRING:
			log_trace("String (\"%s\") ", atom.string_val);
			break;
		case ATOM_EXPR:
			log_trace("Begin expression");
			for (size_t i = 0; i < buffer_len(atom.expr); i++)
				print_expr(atom.expr[i]);
			log_trace("End expression");

			break;
	}
}

// Take next token, skipping whitespace
token_t parser_next() {
	token_t next = lexer_next();
	if (next.kind == TOKEN_SPACE)
		next = lexer_next();
	return next;
}

atom_t parse_item(token_t);

// Parse an expression
atom_t parse_expr() {
	atom_t expr;
	expr.kind = ATOM_EXPR;
	expr.expr = NULL;
	
	for (;;) {
		token_t next = parser_next();

		if (next.kind == TOKEN_EOF)
			error(1, "Expected ')', found EOF");
		else if (next.kind == TOKEN_RPAREN)
			break;

		buffer_push(expr.expr, parse_item(next));
	}

	return expr;
}

// Parse a top-level file (the same as parsing an expression except without the ending ')')
atom_t parse() {
	log_info("Begin parsing");
	atom_t atom;
	atom.kind = ATOM_EXPR;
	atom.expr = NULL;

	for (;;) {
		token_t next = parser_next();

		if (next.kind == TOKEN_EOF)
			break;

		atom_t expr = parse_item(next);
		buffer_push(atom.expr, expr);
	}

	log_info("Parsing complete");

	return atom;
}

// Parse individual item
atom_t parse_item(token_t next) {
	atom_t atom;
	
	switch (next.kind) {
		case TOKEN_LPAREN: 
			atom = parse_expr(TOKEN_LPAREN); 
			break;
		case TOKEN_INT: 
			atom.kind = ATOM_INTEGER;
			atom.integer_val = next.int_val;
			break;
		case TOKEN_FLOAT: 
			atom.kind = ATOM_FLOAT;
			atom.float_val = next.float_val;
			break;
		case TOKEN_STRING: 
			atom.kind = ATOM_STRING;
			char *str = next.string_val;

			// Make sure the string is valid (starts and ends with a quotation mark)
			assert(str[0] == '"');
			assert(str[strlen(str) - 1] == '"');

			// Reallocate the string, excluding the start and end '"'
			// TODO: make this more efficient (somehow)
			atom.string_val = intern_range(str + 1, str + strlen(str) - 1);
			break;
		case TOKEN_SYMBOL: 
			atom.kind = ATOM_SYMBOL;
			atom.symbol_val = next.symbol_val;
			break;
	}

	return atom;
}
