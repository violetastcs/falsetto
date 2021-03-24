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

typedef struct record_entry {
	uint64_t hash;
	record_t record;
} record_entry_t;

buffer_t(record_entry_t) records;

void record_def(record_t record) {
	uint64_t hash = str_hash(record.name);

	for (size_t i = 0; i < buffer_len(records); i++) {
		if (records[i].hash == hash)
			error(1, "Record %s already defined");
	}

	record_entry_t entry;
	entry.hash = hash;
	entry.record = record;

	buffer_push(records, entry);
}

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
	log_trace("Get type of %s", name);

	type_t t;
	t.kind = TYPE_VOID;

	uint64_t hash = str_hash(name);
	
	for (size_t i = 0; i < buffer_len(list); i++) {
		item_type_info_t info = list[i];

		if (info.hash == hash) {
			t = info.type;
			break;
		}
	}

	return t;
}

item_type_info_t *types_getp(type_list_t list, char *name) {
	uint64_t hash = str_hash(name);
	
	for (size_t i = 0; i < buffer_len(list); i++) {
		item_type_info_t info = list[i];

		if (info.hash == hash) {
			return list + i;;
		}
	}

	return NULL;
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

bool type_coerces(type_t to, type_t from, type_list_t *list, char *symbol_name) {
	bool coerces;
	
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
			coerces = from.kind == TYPE_INTEGER or from.kind == to.kind; 
			break;

		case TYPE_ARRAY:
			if (from.kind == TYPE_ARRAY and to.count == from.count)
				coerces = type_coerces(*to.child, *from.child, list, symbol_name);
			else 
				coerces = false;
			break;

		case TYPE_POINTER:
			if (from.kind == TYPE_POINTER)
				coerces = type_coerces(*to.child, *from.child, list, symbol_name);
			else
				coerces = false;
			break;

		default:
			coerces = to.kind == from.kind;
			break;
	}

	if (coerces && symbol_name != NULL) {
		assert(list != NULL);
		item_type_info_t *i = types_getp(*list, symbol_name);

		if (i == NULL)
			error(1, "Variable %s not found");

		i->type = to;
	}

	return coerces;
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
	"%s *aref%s(%s *a,long long int i){"
		"return (a->inner + i);"
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

type_t type_of_expr(type_list_t *types, ast_expr_t expr);

type_t type_of_call(type_list_t *types, ast_call_t call) {
	func_type_info_t info = get_func_def(call.name);

	if (info.hash == 0)
		error(1, "Unknown function %s", call.name);

	size_t argc = buffer_len(info.args);
	size_t argp = buffer_len(call.args);

	if (argp != argc && !info.vararg)
		error(1, "Function %s expects %d arguments, found %d", call.name, argc, argp);

	for (size_t i = 0; i < argp; i++) {
		type_t type = type_of_expr(types, call.args[i]);

		char *symb = NULL;
		if (call.args[i].kind == AST_EXPR_SYMBOL)
			symb = call.args[i].symbol_val;

		if (!type_coerces(info.args[i], type, types, symb) && i < argc) {
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

type_t type_of_expr(type_list_t *types, ast_expr_t expr) {
	type_t expr_type;

	switch (expr.kind) {
		case AST_EXPR_SYMBOL:
			expr_type = types_get(*types, expr.symbol_val);
			log_trace("Symbol %s is type %s", expr.symbol_val, type_as_string(expr_type));
			break;
		case AST_EXPR_STRING: {
			type_t t;
			t.kind = TYPE_POINTER;
			t.child = malloc(sizeof(type_t));
			*t.child = type_kind(TYPE_U8);
			expr_type = t;
			break;
		}
		case AST_EXPR_INTEGER:
			expr_type = type_kind(TYPE_INTEGER);
			break;
		case AST_EXPR_FLOAT:
			TODO("Implement floating point values");
			break;
		case AST_EXPR_BOOL:
			expr_type = type_kind(TYPE_BOOL);
			break;

		case AST_EXPR_CALL: {
			expr_type = type_of_call(types, expr.call);
			break;
		}

		case AST_EXPR_CAST: {
			type_t from = type_of_expr(types, *expr.cast.from);
			if (!type_casts(expr.cast.to, from))
				error(1, "Cannot cast from type %s to type %s", type_as_string(from), type_as_string(expr.cast.to));
			expr_type = expr.cast.to;
		
			break;
		}

		case AST_EXPR_ARRAY: {
			type_t type1 = type_of_expr(types, expr.array[0]);

			for (size_t i = 1; i < buffer_len(expr.array); i++) {
				type_t typei = type_of_expr(types, expr.array[i]);

				char *symb = NULL;
				if (expr.array[i].kind == AST_EXPR_SYMBOL)
					symb = expr.array[i].symbol_val;
				
				if (!type_coerces(typei, type1, types, symb))
					error(1, "Item %ld of array expected '%s', found '%s'", i, type_as_string(type1), type_as_string(typei));
			}

			type_t arr;
			arr.kind = TYPE_ARRAY;
			arr.count = buffer_len(expr.array);
			arr.child = malloc(sizeof(type_t));
			*arr.child = type1;

			expr_type = arr;
			break;
		}

		case AST_EXPR_GET: {
			type_t ptr = type_of_expr(types, *expr.get.ptr);

        		if (ptr.kind != TYPE_POINTER)
				error(1, "Get expects pointer, found %s", type_as_string(ptr));

			expr_type = *ptr.child;
			break;
		}


		case AST_EXPR_REF: {
			type_t var = types_get(*types, expr.ref.var);

			if (var.kind == TYPE_VOID)
				error(1, "Variable %s not found", expr.ref.var);

			expr_type = type_ptr(var);
			break;
		}

		case AST_EXPR_AREF: {
			type_t array = type_of_expr(types, *expr.aref.array);
			type_t index = type_of_expr(types, *expr.aref.index);

			if (array.kind != TYPE_ARRAY)
				error(1, "Aref expects Array, found %s", type_as_string(array));

			if (!is_integer(index))
				error(1, "Aref expects integer index, found %s", type_as_string(index));

			expr_type = type_ptr(*array.child);
			break;
		}

		case AST_EXPR_UNIOP:
			switch (expr.unop.kind) {
				case AST_UNOP_NOT: {}
					ast_expr_t arg = *expr.unop.arg;
					type_t type = type_of_expr(types, arg);

					char *symb = NULL;
					if (arg.kind == AST_EXPR_SYMBOL)
						symb = expr.symbol_val;
					
					if (!type_coerces(type, type_kind(TYPE_BOOL), types, symb))
						error(1, "not expects Bool, found %s", type_as_string(type));

					expr_type = type;
					
					break;
			}

			break;

		case AST_EXPR_BINOP: {}
			ast_expr_t lhs_expr = *expr.binop.args[0];
			ast_expr_t rhs_expr = *expr.binop.args[1];
			type_t lhs = type_of_expr(types, lhs_expr);
			type_t rhs = type_of_expr(types, lhs_expr);

			char *lhs_symb = NULL;
			char *rhs_symb = NULL;

			if (lhs_expr.kind == AST_EXPR_SYMBOL)
				lhs_symb = lhs_expr.symbol_val;
			if (rhs_expr.kind == AST_EXPR_SYMBOL)
				rhs_symb = rhs_expr.symbol_val;

			if (!type_coerces(lhs, rhs, types, lhs_symb) or !type_coerces(rhs, lhs, types, rhs_symb))
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

					if (!type_coerces(lhs, rhs, types, lhs_symb) and !type_coerces(rhs, lhs, types, lhs_symb))
						error(1, "Comparison operator must be applied on equal types");
					
					break;

				case AST_BINOP_AND:
				case AST_BINOP_OR:
					if (!type_coerces(lhs, type_kind(TYPE_BOOL), types, lhs_symb))
						error(1, "Boolean operator expected Bool, found %s", type_as_string(lhs));
					if (!type_coerces(rhs, type_kind(TYPE_BOOL), types, lhs_symb))
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
			case AST_STATEMENT_DECL: {}
				type_t exists = types_get(ty, st.decl.name);
				if (exists.kind != TYPE_VOID)
					error(1, "Attempted to redeclare variable %s", st.decl.name);
				
				types_add(&ty, st.decl.name, st.decl.type);
				log_trace("Declaration type: %s", type_as_string(st.decl.type));
				def_type(st.decl.type);
				break;

			case AST_STATEMENT_SET: {}
				type_t type_var = types_get(ty, st.set.name);

				type_t type_exp = type_of_expr(&ty, st.set.val);

				log_trace("Variable %s is type %s", st.set.name, type_as_string(type_var));

				if (!type_coerces(type_var, type_exp, &ty, st.set.name))
					error(1, "Variable %s is of type %s, found %s", st.set.name, type_as_string(type_var), type_as_string(type_exp));
				else 
					*st.set.val.type = type_var;

				break;

			case AST_STATEMENT_LET: {
				type_t exists = types_get(ty, st.let.name);
				//type_t exists;
				if (exists.kind != TYPE_VOID)
					error(1, "Attempted to redeclare variable %s", st.let.name);

				type_t expr = type_of_expr(&ty, st.let.val);
				types_add(&ty, st.let.name, expr);
				log_trace("Let type: %s", type_as_string(expr));
				def_type(expr);
				break;
			}

			case AST_STATEMENT_CFLOW: {
				type_t type_flw = type_of_expr(&ty, st.cflow.cond);

				char *symb = NULL;
				if (st.cflow.cond.kind == AST_EXPR_SYMBOL)
					symb = st.cflow.cond.symbol_val;

				if (!type_coerces(type_flw, type_kind(TYPE_BOOL), &ty, symb))
					error(1, "Control flow condition expected bool, found %s", type_as_string(type_flw));

				check_body(ret, ty, st.cflow.body);
				break;
			}

			case AST_STATEMENT_STORE: {
				type_t ptr = type_of_expr(&ty, st.store.ptr);
				type_t val = type_of_expr(&ty, st.store.val);

				char *symb = NULL;
				if (st.store.val.kind == AST_EXPR_SYMBOL)
					symb = st.store.val.symbol_val;

				if (ptr.kind != TYPE_POINTER)
					error(1, "Store expects pointer, found %s", type_as_string(ptr));

				if (!type_coerces(*ptr.child, val, &ty, symb))
					error(1, "Store expected %s, found %s", type_as_string(*ptr.child), type_as_string(val));

				break;
			}

			case AST_STATEMENT_RETURN: {
				type_t type_ret = type_of_expr(&ty, st.ret);

				char *symb = NULL;
				if (st.ret.kind == AST_EXPR_SYMBOL)
					symb = st.ret.symbol_val;

				if (!type_coerces(ret, type_ret, &ty, symb))
					error(1, "Expected return type %s, found %s", type_as_string(ret), type_as_string(type_ret));

				break;
			}

			case AST_STATEMENT_CALL: 
				type_of_call(&ty, st.call);
				break;
		}
	}

	for (size_t i = 0; i < buffer_len(body); i++) {
		ast_statement_t st = body[i];

		if (st.kind == AST_STATEMENT_LET and is_partial(*st.let.val.type)) {
			type_t type = types_get(ty, st.let.name);

			if (type.kind == TYPE_VOID)
				error(1, "Internal compiler error: variable %s has type TYPE_VOID", st.let.name);

			if (is_partial(type))
				error(1, "Not enough info to infer type of variable %s", st.let.name);

			log_trace("Inferred type %s for variable %s", type_as_string(type), st.let.name);

			*st.let.val.type = type;
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

			case AST_TL_RECORD:
				record_def(tl.record);

				for (size_t i = 0; i < buffer_len(tl.record.fields); i++)
					def_type(tl.record.fields[i].type);

				break;

			default: break;
		}
	}

	assert(!type_coerces(type_kind(TYPE_U8), type_kind(TYPE_I32)));

	assert(is_partial(type_kind(TYPE_INTEGER)));
	assert(!is_partial(type_kind(TYPE_I64)));

	log_info("End type checking");
}
