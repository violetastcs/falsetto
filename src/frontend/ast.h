#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdio.h>

#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/parser.h>

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

typedef enum ast_expr_kind {
	AST_EXPR_BINOP,
	AST_EXPR_UNIOP,
	AST_EXPR_SYMBOL,
	AST_EXPR_STRING,
	AST_EXPR_INTEGER,
	AST_EXPR_FLOAT,
	AST_EXPR_CALL
} ast_expr_kind_t;

typedef struct ast_expr {
	ast_expr_kind_t kind;

	union {
		ast_binop_t binop;
		ast_unop_t unop;
		ast_call_t call;
		int64_t int_val;
		char *string_val;
		char *symbol_val;
		double float_val;
	}
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
		case ATOM_SYMBOL:
			e.kind = AST_EXPR_SYMBOL;
			e.symbol_val = expr.symbol_val;
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

			} else {
				e.kind = AST_EXPR_CALL;
				e.call.name = op;
				e.call.args = NULL;

				for (size_t i = 1; i < buffer_len(expr.expr); i++) 
					buffer_push(e.call.args, parse_ast_expr(expr.expr[i]));
			}
			
			break;
	}

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
	char *type;
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
	}
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

			if (!is_symbol(atom.expr[2], NULL))
				TODO("Implement type checking");

			st.decl.name = atom.expr[1].symbol_val;
			st.decl.type = atom.expr[2].symbol_val;

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
	char *type;
} ast_arg_t;

buffer_t(ast_arg_t) parse_args(atom_t args) {
	buffer_t(ast_arg_t) list = NULL;

	if (args.kind != ATOM_EXPR)
		error(1, "Argument list must be an expression");
	
	for (size_t i = 0; i < buffer_len(args.expr); i++) {
		atom_t name = args.expr[i].expr[0];
		atom_t type = args.expr[i].expr[1];

		if (!is_symbol(name, NULL))
			error(1, "Argument name must be a symbol");

		if (!is_symbol(type, NULL)) 
			error(1, "Argument type must be a symbol (TODO: type checking)");

		ast_arg_t arg;

		arg.name = name.symbol_val;
		arg.type = type.symbol_val;

		buffer_push(list, arg);
	}

	return list;
}

typedef struct ast_func {
	char *name;
	buffer_t(ast_arg_t) args;
	char *ret;
	buffer_t(ast_statement_t) body;
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
	}
} ast_tl_t;

typedef struct ast_program {
	buffer_t(ast_tl_t) items;
} ast_program_t;

ast_program_t parse_program(atom_t program) {
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

			log_trace("Valid include!");
		} else if (strcmp(symbol, "func") == 0) {
			item.kind = AST_TL_FUNC;

			ast_func_t func;

			if (!is_symbol(expr.expr[1], NULL)) 
				error(1, "Function identifier must be a symbol");
			func.name = intern_str(expr.expr[1].symbol_val);

			func.args = parse_args(expr.expr[2]);

			if (!is_symbol(expr.expr[3], NULL))
				error(1, "Function return type must be a symbol");

			func.ret = intern_str(expr.expr[3].symbol_val);

			func.body = NULL;

			if (buffer_len(expr.expr) == 4)
				continue;
			else if (buffer_len(expr.expr) == 5) {
				func.body = parse_body(expr.expr[4]);
			} else 
				error(1, "Invalid argument count to func");

			item.func = func;
		}
		buffer_push(prog.items, item);
	}

	return prog;
}
