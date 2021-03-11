#pragma once

#include <stdint.h>
#include <stddef.h>

#include <utils/intern.h>
#include <utils/log.h>
#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/parser.h>

typedef enum type_kind {
	TYPE_INTEGER,
	TYPE_STRING,
	TYPE_ARRAY,
	TYPE_POINTER
} type_kind_t;

typedef struct type {
	type_kind_t kind;
	struct type *child;
} type_t;

type_t parse_type(atom_t type_expr) {
	type_t type;

	if (is_symbol(type_expr, "I64")) {
		type.kind = TYPE_INTEGER;
		type.child = NULL;
	} else if (is_symbol(type_expr, "String")) {
		type.kind = TYPE_STRING;
		type.child = NULL;
	} else if (type_expr.kind == ATOM_EXPR) {
		if (is_symbol(type_expr.expr[0], "Array"))
			type.kind = TYPE_ARRAY;
		else if (is_symbol(type_expr.expr[0], "Pointer"))
			type.kind == TYPE_ARRAY;
		else 
			error(1, "Expected 'Array' or 'Pointer'");

		type.child = malloc(sizeof(type_t));
		*type.child = parse_type(type_expr.expr[1]);
	}
}
