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

#include <stdint.h>
#include <stddef.h>

#include <utils/intern.h>
#include <utils/log.h>
#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/ast.h>

buffer_t(uint64_t) def_hashes = NULL;
buffer_t(char *) defs = NULL;

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
	bool vararg;
} func_type_info_t;

buffer_t(func_type_info_t) func_defs = NULL;

void add_func_def(ast_func_t func) {
	func_type_info_t f;

	f.hash = str_hash(func.name);
	f.args = NULL;
	f.ret = func.ret;
	f.vararg = func.vararg;

	for (size_t i = 0; i < buffer_len(func.args); i++)
		buffer_push(f.args, func.args[i].type);

	buffer_push(func_defs, f);
}

func_type_info_t get_func_def(char *name) {
	func_type_info_t f = { 0 };
	uint64_t hash = str_hash(name);

	for (size_t i = 0; i < buffer_len(func_defs); i++) {
		if (func_defs[i].hash == hash) { 
			f = func_defs[i];
			break;
		}
	}

	return f;
}

char *array_template = 
	"typedef struct{%s inner[%ld];}%s;"
	"%s get%s(%s a,int i){"
		"return a.inner[i];"
	"}"
;

char *array_gen(type_t type) {
	char *ctype = type_to_str(*type.child);
	char *mangle = type_mangle(type);
	return heap_fmt(array_template, ctype, type.count, mangle, ctype, mangle, mangle);
}

void def_type(type_t type) {
	if (type.kind == TYPE_ARRAY and !is_partial(*type.child)) {
		
		char *mangled = type_mangle(type);
		uint64_t hash = str_hash(mangled);
		bool found = false;

		for (size_t i = 0; i < buffer_len(def_hashes); i++) 
			if (def_hashes[i] == hash)
				found = true;

		if (!found) {
			def_type(*type.child);
			char *gen = array_gen(type);
			buffer_push(defs, gen);
			buffer_push(def_hashes, str_hash(mangled));
		}
	}
}

type_t type_of_expr(type_list_t types, ast_expr_t expr);

type_t type_of_call(type_list_t types, ast_call_t call) {
	func_type_info_t info = get_func_def(call.name);

	if (info.hash == 0)
		error(1, "Unknown function %s", call.name);

	size_t argc = buffer_len(info.args);
	size_t argp = buffer_len(call.args);

	if (argp != argc && !info.vararg)
		error(1, "Function %s expects %d arguments, found %d", call.name, argc, argp);

	for (size_t i = 0; i < argp; i++) {
		type_t type = type_of_expr(types, call.args[i]);

		if (!type_coerces(type, info.args[i]) && i < argc) {
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
	type_t expr_type;

	switch (expr.kind) {
		case AST_EXPR_SYMBOL:
			expr_type = types_get(types, expr.symbol_val);
			log_trace("Symbol %s is type %s", expr.symbol_val, type_as_string(expr_type));
			break;
		case AST_EXPR_STRING: {}
			type_t t;
			t.kind = TYPE_POINTER;
			t.child = malloc(sizeof(type_t));
			*t.child = type_kind(TYPE_U8);
			expr_type = t;
			break;
		case AST_EXPR_INTEGER:
			expr_type = type_kind(TYPE_INTEGER);
			break;
		case AST_EXPR_FLOAT:
			TODO("Implement floating point values");
			break;
		case AST_EXPR_BOOL:
			expr_type = type_kind(TYPE_BOOL);
			break;

		case AST_EXPR_CALL: {}
			expr_type = type_of_call(types, expr.call);
			break;

		case AST_EXPR_CAST: {}
			type_t from = type_of_expr(types, *expr.cast.from);
			if (!type_casts(expr.cast.to, from))
				error(1, "Cannot cast from type %s to type %s", type_as_string(from), type_as_string(expr.cast.to));
			expr_type = expr.cast.to;
		
			break;

		case AST_EXPR_ARRAY: {}
			type_t type1 = type_of_expr(types, expr.array[0]);

			for (size_t i = 1; i < buffer_len(expr.array); i++) {
				type_t typei = type_of_expr(types, expr.array[i]);
				if (!type_coerces(typei, type1))
					error(1, "Item %ld of array expected '%s', found '%s'", i, type_as_string(type1), type_as_string(typei));
			}

			type_t arr;
			arr.kind = TYPE_ARRAY;
			arr.count = buffer_len(expr.array);
			arr.child = malloc(sizeof(type_t));
			*arr.child = type1;

			expr_type = arr;
			break;

		case AST_EXPR_GET: {}
			type_t array = type_of_expr(types, *expr.get.array);
			type_t index = type_of_expr(types, *expr.get.index);

        		if (array.kind != TYPE_ARRAY)
				error(1, "Get expects Array, found %s", type_as_string(array));

			if (!is_integer(index))
				error(1, "Get expects index  to be integer, found %s", type_as_string(index));

			expr_type = *array.child;
			break;

		case AST_EXPR_UNIOP:
			switch (expr.unop.kind) {
				case AST_UNOP_NOT: {}
					type_t type = type_of_expr(types, *expr.unop.arg);
					if (!type_coerces(type, type_kind(TYPE_BOOL)))
						error(1, "not expects Bool, found %s", type_as_string(type));

					expr_type = type;
					
					break;
			}

			break;

		case AST_EXPR_BINOP: {}
			type_t lhs = type_of_expr(types, *expr.binop.args[0]);
			type_t rhs = type_of_expr(types, *expr.binop.args[1]);

			if (!type_coerces(lhs, rhs)) 
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
					expr_type = lhs;
					break;

				case AST_BINOP_EQ:
				case AST_BINOP_NEQ:
				case AST_BINOP_LT:
				case AST_BINOP_GT:
				case AST_BINOP_LTEQ:
				case AST_BINOP_GTEQ:
					expr_type = type_kind(TYPE_BOOL);
					break;

				case AST_BINOP_AND:
				case AST_BINOP_OR:
					if (!type_coerces(lhs, type_kind(TYPE_BOOL)))
						error(1, "Boolean operator expected Bool, found %s", type_as_string(lhs));
					if (!type_coerces(rhs, type_kind(TYPE_BOOL)))
						error(1, "Boolean operator expected Bool, found %s", type_as_string(rhs));

					expr_type = type_kind(TYPE_BOOL);
					break;
			}

			break;
	}

	def_type(expr_type);

        *expr.type = expr_type;
	return expr_type;
}

void check_body(type_t ret, type_list_t types, buffer_t(ast_statement_t) body) {
	type_list_t ty = types_clone(types);

	for (size_t i = 0; i < buffer_len(body); i++) {
		ast_statement_t st = body[i];

		switch (st.kind) {
			case AST_STATEMENT_DECL:
				types_add(&ty, st.decl.name, st.decl.type);
				log_trace("Declaration type: %s", type_as_string(st.decl.type));
				def_type(st.decl.type);
				break;

			case AST_STATEMENT_SET: {}
				type_t type_var = types_get(ty, st.set.name);

				type_t type_exp = type_of_expr(ty, st.set.val);

				log_trace("Variable %s is type %s", st.set.name, type_as_string(type_var));

				if (!type_coerces(type_var, type_exp))
					error(1, "Variable %s is of type %s, found %s", st.set.name, type_as_string(type_var), type_as_string(type_exp));
				else 
					*st.set.val.type = type_var;

				break;

			case AST_STATEMENT_CFLOW: {}
				type_t type_flw = type_of_expr(ty, st.cflow.cond);

				if (!type_coerces(type_flw, type_kind(TYPE_BOOL)))
					error(1, "Control flow condition expected bool, found %s", type_as_string(type_flw));

				check_body(ret, ty, st.cflow.body);
				break;

			case AST_STATEMENT_RETURN: {}
				type_t type_ret = type_of_expr(ty, st.ret);

				if (!type_coerces(ret, type_ret))
					error(1, "Expected return type %s, found %s", type_as_string(ret), type_as_string(type_ret));

				break;

			case AST_STATEMENT_CALL: 
				type_of_call(ty, st.call);
				break;
		}
	}
}

void type_check(ast_program_t program) {
	log_info("Begin type checking");
	type_list_t types = NULL;
	
	for (size_t i = 0; i < buffer_len(program.items); i++) {
		ast_tl_t tl = program.items[i];

		switch (tl.kind) {
			case AST_TL_FUNC:
				add_func_def(tl.func);
				break;
			default: break;
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

			default: break;
		}
	}

	assert(!type_coerces(type_kind(TYPE_U8), type_kind(TYPE_I32)));

	log_info("End type checking");
}
