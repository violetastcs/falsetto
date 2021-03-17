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
#include <errno.h>
#include <stdio.h>

#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/parser.h>

typedef enum type_kind {
	TYPE_INTEGER,
	
	TYPE_I8,
	TYPE_U8,

	TYPE_I16,
	TYPE_U16,

	TYPE_I32,
	TYPE_U32,

	TYPE_I64,
	TYPE_U64,

	TYPE_BOOL,
	TYPE_VOID,

	TYPE_POINTER,
	TYPE_ARRAY
} type_kind_t;

char *type_str[] = {
	[TYPE_U8]   = "unsigned char",
	[TYPE_I8]   = "signed char",
	[TYPE_U16]  = "unsigned short",
	[TYPE_I16]  = "short",
	[TYPE_U32]  = "unsigned int",
	[TYPE_I32]  = "int",
	[TYPE_U64]  = "unsigned long long",
	[TYPE_I64]  = "long long",
	[TYPE_BOOL] = "int",
	[TYPE_VOID] = "void"
};

char *type_str_lang[] = {
	[TYPE_U8] = "U8",
	[TYPE_I8] = "I8",
	[TYPE_U16] = "U16",
	[TYPE_I16] = "I16",
	[TYPE_U32] = "U32",
	[TYPE_I32] = "I32",
	[TYPE_U64] = "U64",
	[TYPE_I64] = "I64",

	[TYPE_INTEGER] = "{Integer}",

	[TYPE_BOOL] = "Bool",
	[TYPE_VOID] = "Void",
};

typedef struct type {
	type_kind_t kind;

	size_t count;
	struct type *child;
} type_t;

char *type_as_string(type_t type) {
	switch (type.kind) {
		case TYPE_ARRAY:
			return heap_fmt("(Array %s %d)", type_as_string(*type.child), type.count);
		case TYPE_POINTER:
			return heap_fmt("(@ %s)", type_as_string(*type.child));

		default:
			return type_str_lang[type.kind];
	}
}

char *type_mangle(type_t type) {
	switch (type.kind) {
		case TYPE_ARRAY:
			return heap_fmt("_Array%s_%d", type_mangle(*type.child), type.count);
		case TYPE_POINTER:
			return heap_fmt("_Pointer%s", type_mangle(*type.child));
		default:
			return heap_fmt("_%s", type_str_lang[type.kind]);
	}
}

char *type_to_str(type_t type) {
	switch (type.kind) {
		case TYPE_ARRAY:
			return type_mangle(type);
		case TYPE_POINTER:
			return heap_fmt("%s*", type_to_str(*type.child));
		default:
			return type_str[type.kind];

	}
}


bool is_partial(type_t type) {
	switch (type.kind) {
		case TYPE_INTEGER:
			return true;

		case TYPE_POINTER:
		case TYPE_ARRAY:
			return is_partial(*type.child);

		default:
			return false;
	}
	
	return type.kind == TYPE_INTEGER;
}

bool is_integer(type_t type) {
	return 
		   type.kind == TYPE_I8
		or type.kind == TYPE_U8
		or type.kind == TYPE_I16
		or type.kind == TYPE_U16
		or type.kind == TYPE_I32
		or type.kind == TYPE_U32
		or type.kind == TYPE_I64
		or type.kind == TYPE_U64
		or type.kind == TYPE_INTEGER
	;
}
bool type_casts(type_t to, type_t from) {
	if (to.kind == from.kind)
		return true;

	switch (to.kind) {
		case TYPE_I8:
		case TYPE_U8:
		case TYPE_I16:
		case TYPE_U16:
		case TYPE_I32:
		case TYPE_U32:
		case TYPE_I64:
		case TYPE_U64:
		case TYPE_INTEGER:
			return is_integer(from);

		default:
			return false;
	}
}

bool type_coerces(type_t to, type_t from) {
	switch (to.kind) {
		case TYPE_I8:
		case TYPE_U8:
		case TYPE_I16:
		case TYPE_U16:
		case TYPE_I32:
		case TYPE_U32:
		case TYPE_I64:
		case TYPE_U64:
		case TYPE_INTEGER:
			return from.kind == TYPE_INTEGER or from.kind == to.kind; 
			break;

		case TYPE_ARRAY:
			if (from.kind == TYPE_ARRAY and to.count == from.count)
				return type_coerces(*to.child, *from.child);
			else 
				return false;
			break;

		case TYPE_POINTER:
			if (from.kind == TYPE_POINTER)
				return type_coerces(*to.child, *from.child);
			else
				return false;
			break;

		default:
			return to.kind == from.kind;
			break;
	}
}

type_t type_kind(type_kind_t kind) {
	type_t t;
	t.kind = kind;
	return t;
}

type_t type_ptr(type_t type) {
	type_t t;
	t.kind = TYPE_POINTER;
	t.child = malloc(sizeof(type_t));
	*t.child = type;
	return t;
}

bool is_string(type_t type) {
	return type.kind == TYPE_POINTER and type.child->kind == TYPE_U8;
}

bool type_cmp(type_t lhs, type_t rhs) {
	if (lhs.kind == TYPE_POINTER && rhs.kind == TYPE_POINTER)
		return type_cmp(*lhs.child, *rhs.child);

	else if (lhs.kind == TYPE_ARRAY && rhs.kind == TYPE_ARRAY)
		return type_cmp(*lhs.child, *rhs.child) && lhs.count == rhs.count;

	else 
		return 
			   (is_integer(lhs) and is_integer(rhs)) 
			or (is_string(lhs) and is_string(rhs))
			or rhs.kind == lhs.kind
		;
}

type_t parse_type(atom_t type) {
	type_t t;

	switch (type.kind) {
		case ATOM_SYMBOL:
			{}
			char *sym = type.symbol_val;

			if (strcmp(sym, "I8") == 0) {
				t.kind = TYPE_I8;
			}
			 else if (strcmp(sym, "U8") == 0) {
				t.kind = TYPE_U8;
			}
			else if (strcmp(sym, "I16") == 0) {
				t.kind = TYPE_I16;
			}
			else if (strcmp(sym, "U16") == 0) {
				t.kind = TYPE_U16;
			}
			else if (strcmp(sym, "I32") == 0) {
				t.kind = TYPE_I32;
			}
			else if (strcmp(sym, "U32") == 0) {
				t.kind = TYPE_U32;
			}
			else if (strcmp(sym, "I64") == 0) {
				t.kind = TYPE_I64;
			}
			 else if (strcmp(sym, "U64") == 0) {
				t.kind = TYPE_U64;
			} else if (strcmp(sym, "Void") == 0) {
				t.kind = TYPE_VOID;
			} else if (strcmp(sym, "Bool") == 0) {
				t.kind = TYPE_BOOL;
			}
			else 
				error(1, "Unknown type: %s", sym);
			
			break;

		case ATOM_EXPR:
			if (is_symbol(type.expr[0], "Array")) {
				t.kind = TYPE_ARRAY;
				t.child = malloc(sizeof(type_t));
				*t.child = parse_type(type.expr[1]);

				if (type.expr[2].kind != ATOM_INTEGER) 
					error(1, "Array length must be integer");

				t.count = (size_t)type.expr[2].integer_val;
			} else if (is_symbol(type.expr[0], "@")) {
				t.kind = TYPE_POINTER;
				t.child = malloc(sizeof(type_t));
				*t.child = parse_type(type.expr[1]);
			} else 
				error(1, "Invalid type modifier");
			break;

		default:
			error(1, "Type must be an expression or symbol");
			break;
	}

	log_trace("Parsed type: %s", type_as_string(t));
	
	return t;
}

typedef enum ast_binop_kind {
	AST_BINOP_ADD = 1,
	AST_BINOP_SUB,
	AST_BINOP_MUL,
	AST_BINOP_DIV,
	AST_BINOP_MOD,

	AST_BINOP_EQ,
	AST_BINOP_NEQ,
	AST_BINOP_LT,
	AST_BINOP_GT,
	AST_BINOP_LTEQ,
	AST_BINOP_GTEQ,

	AST_BINOP_AND,
	AST_BINOP_OR
} ast_binop_kind_t;

typedef enum ast_unop_kind {
	AST_UNOP_NOT = 1
} ast_unop_kind_t;

typedef struct ast_binop {
	ast_binop_kind_t kind;
	struct ast_expr *args[2];
} ast_binop_t;

typedef struct ast_unop {
	ast_unop_kind_t kind;
	struct ast_expr *arg;
} ast_unop_t;

typedef struct ast_call {
	char *name;
	buffer_t(struct ast_expr) args;
} ast_call_t;

typedef struct ast_get {
	struct ast_expr *array;
	struct ast_expr *index;
} ast_get_t;

typedef struct ast_cast {
	type_t to;
	struct ast_expr *from;
} ast_cast_t;

typedef enum ast_expr_kind {
	AST_EXPR_BINOP,
	AST_EXPR_UNIOP,
	AST_EXPR_SYMBOL,
	AST_EXPR_STRING,
	AST_EXPR_INTEGER,
	AST_EXPR_FLOAT,
	AST_EXPR_BOOL,
	AST_EXPR_ARRAY,
	AST_EXPR_GET,
	AST_EXPR_CALL,
	AST_EXPR_CAST
} ast_expr_kind_t;

typedef struct ast_expr {
	ast_expr_kind_t kind;
	type_t *type;

	union {
		ast_binop_t binop;
		ast_unop_t unop;
		ast_call_t call;
		ast_get_t get;
		ast_cast_t cast;
		int64_t int_val;
		char *string_val;
		char *symbol_val;
		double float_val;
		bool bool_val;
		buffer_t(struct ast_expr) array;
	};
} ast_expr_t;

ast_expr_t parse_ast_expr(atom_t expr) {
	ast_expr_t e;

	switch (expr.kind) {
		case ATOM_INTEGER:
			e.kind = AST_EXPR_INTEGER;
			e.int_val = expr.integer_val;
			break;
		case ATOM_STRING:
			e.kind = AST_EXPR_STRING;
			e.string_val = expr.string_val;
			break;
		case ATOM_SYMBOL:{}
			char *sym = expr.symbol_val;

			if (strcmp(sym, "true") == 0) {
				e.kind = AST_EXPR_BOOL;
				e.bool_val = true;
			} else if (strcmp(sym, "false") == 0) {
				e.kind = AST_EXPR_BOOL;
				e.bool_val = false;
			} else {
				e.kind = AST_EXPR_SYMBOL;
				e.symbol_val = expr.symbol_val;
			}
			break;
		case ATOM_FLOAT:
			e.kind = AST_EXPR_FLOAT;
			e.float_val = expr.float_val;
			break;
		case ATOM_EXPR:
			if (!is_symbol(expr.expr[0], NULL))
				error(1, "Expression expected symbol");

			char *op = expr.expr[0].symbol_val;

			if (strcmp(op, "+") == 0) {
        			e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_ADD;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "-") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_SUB;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "*") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_MUL;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "/") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_DIV;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "mod") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_MOD;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "and") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_AND;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "or") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_OR;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "=") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_EQ;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "<") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_LT;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, ">") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_GT;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "<=") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_LTEQ;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, ">=") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_GTEQ;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "!=") == 0) {
				e.kind = AST_EXPR_BINOP;
				e.binop.kind = AST_BINOP_NEQ;

				e.binop.args[0] = malloc(sizeof(ast_expr_t));
				e.binop.args[1] = malloc(sizeof(ast_expr_t));

				*e.binop.args[0] = parse_ast_expr(expr.expr[1]);
				*e.binop.args[1] = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "not") == 0) {
				e.kind = AST_EXPR_UNIOP;				
				e.unop.kind = AST_UNOP_NOT;
				e.unop.arg = malloc(sizeof(ast_unop_t));
				*e.unop.arg = parse_ast_expr(expr.expr[1]);

			}  else if (strcmp(op, "array") == 0) {
				e.kind = AST_EXPR_ARRAY;
				e.array = NULL;

				for (size_t i = 1; i < buffer_len(expr.expr); i++)
					buffer_push(e.array, parse_ast_expr(expr.expr[i]));

			} else if (strcmp(op, "get") == 0) {
				e.kind = AST_EXPR_GET;

				e.get.array = malloc(sizeof(ast_expr_t));
				e.get.index = malloc(sizeof(ast_expr_t));

				*e.get.array = parse_ast_expr(expr.expr[1]);
				*e.get.index = parse_ast_expr(expr.expr[2]);

			} else if (strcmp(op, "cast") == 0 ) {
				e.kind = AST_EXPR_CAST;

				e.cast.from = malloc(sizeof(ast_expr_t));

				*e.cast.from = parse_ast_expr(expr.expr[1]);
				e.cast.to    = parse_type(expr.expr[2]);
				
			} else {
				e.kind = AST_EXPR_CALL;
				e.call.name = op;
				e.call.args = NULL;

				for (size_t i = 1; i < buffer_len(expr.expr); i++) 
					buffer_push(e.call.args, parse_ast_expr(expr.expr[i]));
			}

			
			break;
	}

	e.type = malloc(sizeof(type_t));

	return e;
}

typedef enum ast_cflow_kind {
	AST_CFLOW_WHILE,
	AST_CFLOW_IF
} ast_cflow_kind_t;

typedef struct ast_cflow {
	ast_cflow_kind_t kind;
	ast_expr_t cond;
	buffer_t(struct ast_statement) body;
} ast_cflow_t;

typedef struct ast_set {
	char *name;
	ast_expr_t val;
} ast_set_t;

typedef struct ast_decl {
	char *name;
	type_t type;
} ast_decl_t;

typedef enum ast_statement_kind {
	AST_STATEMENT_DECL,
	AST_STATEMENT_SET,
	AST_STATEMENT_CFLOW,
	AST_STATEMENT_RETURN,
	AST_STATEMENT_CALL
} ast_statement_kind_t;

typedef struct ast_statement {
	ast_statement_kind_t kind;

	union {
		ast_cflow_t cflow;
		ast_call_t call;
		ast_set_t set;
		ast_decl_t decl;
		ast_expr_t ret;
	};
} ast_statement_t;


buffer_t(ast_statement_t) parse_body(atom_t body) {
	buffer_t(ast_statement_t) list = NULL;

	if (body.kind != ATOM_EXPR)
		error(1, "Function body must be an expression");

	for (size_t i = 0; i < buffer_len(body.expr); i++) {
		atom_t atom = body.expr[i];

		ast_statement_t st;

		if (!is_symbol(atom.expr[0], NULL))
			error(1, "Statement expects symbol as first item");

		char *symbol = atom.expr[0].symbol_val;

		if (strcmp(symbol, "decl") == 0) {
			if (buffer_len(atom.expr) != 3)
				error(1, "Invalid argument count for decl");

			st.kind = AST_STATEMENT_DECL;

			if (!is_symbol(atom.expr[1], NULL))
				error(1, "Variable identifier must be a symbol");

			st.decl.name = atom.expr[1].symbol_val;
			st.decl.type = parse_type(atom.expr[2]);

		} else if (strcmp(symbol, "set") == 0) {
			if (buffer_len(atom.expr) != 3)
				error(1, "Invalid argument count for set");

			st.kind = AST_STATEMENT_SET;

			if (!is_symbol(atom.expr[1], NULL))
				error(1, "Variable identifier must be a symbol");

			st.set.name = atom.expr[1].symbol_val;
			st.set.val  = parse_ast_expr(atom.expr[2]);

		} else if (strcmp(symbol, "return") == 0) {
			if (buffer_len(atom.expr) != 2)
				error(1, "Invalid argument count for set");

			st.kind = AST_STATEMENT_RETURN;

			st.ret = parse_ast_expr(atom.expr[1]);

		} else if (strcmp(symbol, "if") == 0) {
			if (buffer_len(atom.expr) != 3)
				error(1, "Invalid argument count for if");

			st.kind = AST_STATEMENT_CFLOW;

			st.cflow.kind = AST_CFLOW_IF;
			st.cflow.cond = parse_ast_expr(atom.expr[1]);
			st.cflow.body = parse_body(atom.expr[2]);

		} else if (strcmp(symbol, "while") == 0) {
			if (buffer_len(atom.expr) != 3)
				error(1, "Invalid argument count for while");

			st.kind = AST_STATEMENT_CFLOW;

			st.cflow.kind = AST_CFLOW_WHILE;
			st.cflow.cond = parse_ast_expr(atom.expr[1]);
			st.cflow.body = parse_body(atom.expr[2]);

		} else {
			st.kind = AST_STATEMENT_CALL;

			st.call.name = symbol;
			st.call.args = NULL;

			for (size_t i = 1; i < buffer_len(atom.expr); i++) 
				buffer_push(st.call.args, parse_ast_expr(atom.expr[i]));
		}


		buffer_push(list, st);
	}

	return list;
}

typedef struct ast_arg {
	char *name;
	type_t type;
} ast_arg_t;

void ast_print_args(buffer_t(ast_arg_t) args, int tabs) {
	if (buffer_len(args) != 0) {
		for (size_t i = 0; i < buffer_len(args); i++) {
			nlt(tabs);
			printf("%zu", i);
			printf(": name = '%s', type = '%s'", args[i].name, type_as_string(args[i].type));
		}
	} else {
		printf("None");
	}
	printf("\n");
}

buffer_t(ast_arg_t) parse_args(atom_t args, bool *vararg) {
	buffer_t(ast_arg_t) list = NULL;

	*vararg = false;

	if (args.kind != ATOM_EXPR)
		error(1, "Argument list must be an expression");
	
	for (size_t i = 0; i < buffer_len(args.expr); i++) {
		if (is_symbol(args.expr[i], "...")) {
			*vararg = true;
			break;
		} else if (args.expr[i].kind == ATOM_EXPR) {
			atom_t name = args.expr[i].expr[0];
			atom_t type = args.expr[i].expr[1];

			if (!is_symbol(name, NULL))
				error(1, "Argument name must be a symbol");

			ast_arg_t arg;

			arg.name = name.symbol_val;
			arg.type = parse_type(type);

			buffer_push(list, arg);
		} else 
			error(1, "Function argument must be (Name Type) or ...");
	}

	return list;
}

typedef struct ast_func {
	char *name;
	buffer_t(ast_arg_t) args;
	type_t ret;
	buffer_t(ast_statement_t) body;
	bool vararg;
} ast_func_t;

typedef enum ast_tl_kind {
	AST_TL_FUNC,
	AST_TL_INCLUDE
} ast_tl_kind_t;

typedef struct ast_tl {
	ast_tl_kind_t kind;

	union {
		char *inc_file;
		ast_func_t func;
	};
} ast_tl_t;

typedef struct ast_program {
	buffer_t(ast_tl_t) items;
} ast_program_t;

ast_program_t parse_program(atom_t program) {
	log_info("Begin AST generation");
	
	ast_program_t prog;
	prog.items = NULL;

	assert(program.kind == ATOM_EXPR);

	for (size_t i = 0; i < buffer_len(program.expr); i++) {
		if (program.expr[i].kind != ATOM_EXPR)
			error(1, "Expected expression, found atom");
		
		if (!is_symbol(program.expr[i].expr[0], NULL)) 
			error(1, "Top level of program expects declarations");

		atom_t expr = program.expr[i];
		char *symbol = expr.expr[0].symbol_val;

		ast_tl_t item;
		item.kind = AST_TL_FUNC;

		if (strcmp(symbol, "include") == 0) {
			item.kind = AST_TL_INCLUDE;

			if (buffer_len(expr.expr) != 2)
				error(1, "Invalid arguments to include");

			if (expr.expr[1].kind != ATOM_STRING) 
				error(1, "Include expects string");

			item.inc_file = expr.expr[1].string_val;

		} else if (strcmp(symbol, "func") == 0) {
			item.kind = AST_TL_FUNC;

			ast_func_t func;

			if (!is_symbol(expr.expr[1], NULL)) 
				error(1, "Function identifier must be a symbol");

			func.name = intern_str(expr.expr[1].symbol_val);
			func.args = parse_args(expr.expr[2], &func.vararg);
			func.ret = parse_type(expr.expr[3]);
                        func.body = NULL;

			if (buffer_len(expr.expr) == 4) {
				item.func = func;
				goto LOOP_END;
			} else if (buffer_len(expr.expr) == 5) {
				func.body = parse_body(expr.expr[4]);
				item.func = func;
			} else 
				error(1, "Invalid argument count to func");
			
		}
		LOOP_END:
		buffer_push(prog.items, item);
		//ast_print_tl(item);
	}

	log_info("End AST generation");

	return prog;
}
