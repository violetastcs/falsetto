#pragma once

#include <stdint.h>
#include <stddef.h>

#include <utils/intern.h>
#include <utils/log.h>
#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/ast.h>

typedef struct item_type_info {
	uint64_t hash;
	type_t type;
} item_type_info_t;

typedef buffer_t(item_type_info_t) type_list_t;

void types_add(type_list_t *list, char *name, type_t type) {
	buffer_t(item_type_info_t) n = *list;
	
	uint64_t hash = str_hash(name);
	buffer_push(n, (item_type_info_t){ hash, type });

	*list = n;
}

type_t types_get(type_list_t list, char *name) {
	type_t t;
	t.kind = TYPE_VOID;

	uint64_t hash = str_hash(name);
	
	for (size_t i = 0; i < buffer_len(list); i++) {
		item_type_info_t info = list[buffer_len(list) - i - 1];

		if (info.hash == hash) {
			t = info.type;
			break;
		}
	}

	return t;
}

type_list_t types_cat(type_list_t first, type_list_t second) {
	type_list_t new = NULL;

	for (size_t i = 0; i < buffer_len(first); i++) 
		buffer_push(new, first[i]);

	for (size_t i = 0; i < buffer_len(second); i++)
		buffer_push(new, second[i]);

	return new;
}

type_list_t types_clone(type_list_t list) {
	type_list_t new = NULL;

	for (size_t i = 0; i < buffer_len(list); i++) 
		buffer_push(new, list[i]);

	return new;
}

typedef struct func_type_info {
	uint64_t hash;
	type_t ret;
	buffer_t(type_t) args;
} func_type_info_t;

buffer_t(func_type_info_t) func_defs = NULL;

void add_func_def(ast_func_t func) {
	func_type_info_t f;

	f.hash = str_hash(func.name);
	f.args = NULL;
	f.ret = func.ret;

	for (size_t i = 0; i < buffer_len(func.args); i++) {
		if (func.args[i].type.kind == TYPE_VARARG)
			log_info("ADD FUNC DEF VARARG!!!!!!!!!!!!!!");
		
		buffer_push(f.args, func.args[i].type);
	}

	buffer_push(func_defs, f);
}

func_type_info_t get_func_def(char *name) {
	func_type_info_t f = { NULL };
	uint64_t hash = str_hash(name);

	for (size_t i = 0; i < buffer_len(func_defs); i++) {
		if (func_defs[i].hash == hash) { 
			f = func_defs[i];
			break;
		}
	}

	return f;
}

type_t type_of_expr(type_list_t types, ast_expr_t expr);

type_t type_of_call(type_list_t types, ast_call_t call) {
	func_type_info_t info = get_func_def(call.name);

	for (size_t i = 0; i < buffer_len(info.args); i++) 
		if (info.args[i].kind == TYPE_VARARG)
			return info.ret;

	// ONLY WHILE VARARGS ISNT WORKING
	if (strcmp(call.name, "printf") == 0)
		return info.ret;

	size_t argc = buffer_len(info.args);
	size_t argp = buffer_len(call.args);

	if (argp != argc)
		error(1, "Function %s expects %d arguments, found %d", call.name, argc, argp);

	for (size_t i = 0; i < argc; i++) {
		type_t type = type_of_expr(types, call.args[i]);
		
		if (!type_cmp(type, info.args[i])) {
			error(
				1, 
				"Argument %d for %s expected %s, found %s", 
				i, 
				call.name, 
				type_as_string(info.args[i]),
				type_as_string(type)
			);
				
		}
	}

	return info.ret;
}

type_t type_of_expr(type_list_t types, ast_expr_t expr) {
	switch (expr.kind) {
		case AST_EXPR_SYMBOL:
			return types_get(types, expr.symbol_val);
		case AST_EXPR_STRING: {}
			type_t t;
			t.kind = TYPE_POINTER;
			t.child = malloc(sizeof(type_t));
			*t.child = type_kind(TYPE_U8);
			return t;
		case AST_EXPR_INTEGER:
			return type_kind(TYPE_I64);
		case AST_EXPR_FLOAT:
			TODO("Implement floating point values");
		case AST_EXPR_BOOL:
			return type_kind(TYPE_BOOL);

		case AST_EXPR_CALL: {}
			return type_of_call(types, expr.call);
			break;

		case AST_EXPR_UNIOP:
			switch (expr.unop.kind) {
				case AST_UNOP_NOT: {}
					type_t type = type_of_expr(types, *expr.unop.arg);
					if (!type_cmp(type, type_kind(TYPE_BOOL)))
						error(1, "not expects Bool, found %s", type_as_string(type));
					
					break;
			}

			break;

		case AST_EXPR_BINOP: {}
			type_t lhs = type_of_expr(types, *expr.binop.args[0]);
			type_t rhs = type_of_expr(types, *expr.binop.args[1]);

			if (!type_cmp(lhs, rhs)) 
				error(1, "Operands to binary expression must be of the same type");

			switch (expr.binop.kind) {
				case AST_BINOP_ADD:
				case AST_BINOP_SUB:
				case AST_BINOP_MUL:
				case AST_BINOP_DIV:
				case AST_BINOP_MOD:
					if (!is_integer(lhs))
						error(1, "Arithmetic operator expected Integer, found %s", type_as_string(lhs));
					if (!is_integer(rhs))
						error(1, "Arithmetic operator expected Integer, found %s", type_as_string(rhs));
					return lhs;

				case AST_BINOP_EQ:
				case AST_BINOP_NEQ:
				case AST_BINOP_LT:
				case AST_BINOP_GT:
				case AST_BINOP_LTEQ:
				case AST_BINOP_GTEQ:
					return type_kind(TYPE_BOOL);

				case AST_BINOP_AND:
				case AST_BINOP_OR:
					if (!type_cmp(lhs, type_kind(TYPE_BOOL)))
						error(1, "Boolean operator expected Bool, found %s", type_as_string(lhs));
					if (!type_cmp(rhs, type_kind(TYPE_BOOL)))
						error(1, "Boolean operator expected Bool, found %s", type_as_string(rhs));

					return type_kind(TYPE_BOOL);
			}

			break;
	}
}

void check_body(type_t ret, type_list_t types, buffer_t(ast_statement_t) body) {
	type_list_t ty = types_clone(types);

	for (size_t i = 0; i < buffer_len(body); i++) {
		ast_statement_t st = body[i];

		switch (st.kind) {
			case AST_STATEMENT_DECL:
				types_add(&ty, st.decl.name, st.decl.type);
				break;

			case AST_STATEMENT_SET: {}
				type_t type_var = types_get(ty, st.set.name);
				type_t type_exp = type_of_expr(ty, st.set.val);

				if (!type_cmp(type_var, type_exp))
					error(1, "Variable %s is of type %s, found %s", st.set.name, type_as_string(type_var), type_as_string(type_exp));

				break;

			case AST_STATEMENT_CFLOW: {}
				type_t type_flw = type_of_expr(ty, st.cflow.cond);

				if (!type_cmp(type_flw, type_kind(TYPE_BOOL)))
					error(1, "Control flow condition expected bool, found %s", type_as_string(type_flw));

				check_body(ret, ty, st.cflow.body);
				break;

			case AST_STATEMENT_RETURN: {}
				type_t type_ret = type_of_expr(ty, st.ret);

				if (!type_cmp(type_ret, ret))
					error(1, "Expected return type %s, found %s", type_as_string(ret), type_as_string(type_ret));

				break;

			case AST_STATEMENT_CALL: 
				type_of_call(ty, st.call);
				break;
		}
	}
}

void type_check(ast_program_t program) {
	type_list_t types = NULL;
	
	for (size_t i = 0; i < buffer_len(program.items); i++) {
		ast_tl_t tl = program.items[i];

		switch (tl.kind) {
			case AST_TL_FUNC:
				add_func_def(tl.func);
				break;
		}
	}

	for (size_t i = 0; i < buffer_len(program.items); i++) {
		ast_tl_t tl = program.items[i];

		switch (tl.kind) {
			case AST_TL_FUNC:
				for (size_t i = 0; i < buffer_len(tl.func.args); i++)
					types_add(&types, tl.func.args[i].name, tl.func.args[i].type);

				check_body(tl.func.ret, types, tl.func.body);
				break;
		}
	}
}
