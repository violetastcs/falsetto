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

bool is_symbol(atom_t expr, char *name) {
	if (name == NULL)
		return expr.kind == ATOM_SYMBOL;
	else 
		return expr.kind == ATOM_SYMBOL and strcmp(expr.symbol_val, name) == 0;
}

void print_expr(atom_t atom) {
	switch (atom.kind) {
		case ATOM_INTEGER:
			log_trace("INTEGER: %ld ", atom.integer_val);
			break;
		case ATOM_FLOAT:
			log_trace("FLOAT: %f ", atom.float_val);
			break;
		case ATOM_SYMBOL:
			log_trace("SYMBOL: %s ", atom.symbol_val);
			break;
		case ATOM_STRING:
			log_trace("STRING: \"%s\" ", atom.string_val);
			break;
		case ATOM_EXPR:
			log_trace("BEGIN EXPRESSION");
			for (size_t i = 0; i < buffer_len(atom.expr); i++)
				print_expr(atom.expr[i]);
			log_trace("END EXPRESSION");

			break;
	}
}

token_t parser_next() {
	token_t next = lexer_next();
	if (next.kind == TOKEN_SPACE)
		next = lexer_next();
	return next;
}

atom_t parse_item(token_t);

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

atom_t parse() {
	atom_t atom;
	atom.kind = ATOM_EXPR;
	atom.expr = NULL;

	for (;;) {
		token_t next = parser_next();

		if (next.kind == TOKEN_EOF)
			break;
		
		buffer_push(atom.expr, parse_item(next));
	}

	return atom;
}

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

			assert(str[0] == '"');
			assert(str[strlen(str) - 1] == '"');
			
			atom.string_val = intern_range(str + 1, str + strlen(str) - 1);
			break;
		case TOKEN_SYMBOL: 
			atom.kind = ATOM_SYMBOL;
			atom.symbol_val = next.symbol_val;
			break;
	}

	return atom;
}

token_t parser_expect(token_kind_t kind) {
	token_t next = parser_next();

	if (next.kind != kind) 
		error(1, "Expected %d, found %d", kind, next.kind);
	else
		return next;
}
