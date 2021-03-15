#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdio.h>

#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/ast.h>
#include <visitors/type-check.h>

char *binops[] = {
	[AST_BINOP_ADD]  = "+",
	[AST_BINOP_SUB]  = "-",
	[AST_BINOP_MUL]  = "*",
	[AST_BINOP_DIV]  = "/",
	[AST_BINOP_MOD]  = "%",

	[AST_BINOP_EQ]   = "==",
	[AST_BINOP_NEQ]  = "!=",
	[AST_BINOP_LT]   = "<",
	[AST_BINOP_GT]   = ">",
	[AST_BINOP_LTEQ] = "<=",
	[AST_BINOP_GTEQ] = ">=",

	[AST_BINOP_AND]  = "&&",
	[AST_BINOP_OR]   = "||"
};

char *unops[] = {
	[AST_UNOP_NOT]   = "!"
};

void compile_expr(FILE*, ast_expr_t);

void compile_call(FILE *outp, ast_call_t call) {
	log_trace("Compiling function call: name = '%s', argc = %d", call.name, buffer_len(call.args));
	
	fprintf(outp, "%s(", call.name);

	for (size_t i = 0; i < buffer_len(call.args); i++) {
		compile_expr(outp, call.args[i]);

		if (i != buffer_len(call.args) - 1)
			fputc(',', outp);
	}

	fputc(')', outp);
}

void compile_expr(FILE *outp, ast_expr_t expr) {
	switch (expr.kind) {
		case AST_EXPR_SYMBOL:
			fputs(expr.symbol_val, outp);
			break;
		case AST_EXPR_INTEGER:
			fprintf(outp, "%ld", expr.int_val);
			break;
		case AST_EXPR_FLOAT:
			fprintf(outp, "%f", expr.float_val);
			break;
		case AST_EXPR_STRING:
			fprintf(outp, "\"%s\"", expr.string_val);
			break;
		case AST_EXPR_BOOL:
			fputc(expr.bool_val ? '1' : '0', outp);
			break;

		case AST_EXPR_BINOP:
			log_trace("Compiling binary operator expression (AST_EXPR_BINOP)");
			fputc('(', outp);

			compile_expr(outp, *expr.binop.args[0]);
			fputs(binops[expr.binop.kind], outp);
			compile_expr(outp, *expr.binop.args[1]);
			
			fputc(')', outp);
			break;

		case AST_EXPR_UNIOP:
			log_trace("Compiling unary operator expression (AST_EXPR_UNIOP)");
			
			fputc('(', outp);

			fputs(unops[expr.unop.kind], outp);
			compile_expr(outp, *expr.unop.arg);

			fputc(')', outp);
			break;

		case AST_EXPR_ARRAY:
			log_trace("Compiling array expression (AST_EXPR_ARRAY)");

			fprintf(outp, "(%s)", type_mangle(*expr.type));

			fputs("{{", outp);

			for (size_t i = 0; i < buffer_len(expr.array); i++) {
				compile_expr(outp, expr.array[i]);

				if (i != buffer_len(expr.array) - 1) 
					fputc(',', outp);
			}
			
			fputs("}}", outp);
			break;

		case AST_EXPR_GET:
			log_trace("Compiling get expression (AST_EXPR_GET");

			ast_expr_t array = *expr.get.array;
			ast_expr_t index = *expr.get.index;

			fprintf(outp, "get%s(", type_mangle(*array.type));
			compile_expr(outp, array);
			fputc(',', outp);
			compile_expr(outp, index);
			fputc(')', outp);

			break;
	
		case AST_EXPR_CALL:
			compile_call(outp, expr.call);
			break;
	}
}

char *cflows[] = {
	[AST_CFLOW_IF]    = "if",
	[AST_CFLOW_WHILE] = "while"
};

void compile_statement(FILE *outp, ast_statement_t st) {
	switch (st.kind) {
		case AST_STATEMENT_DECL:
			log_trace("Compiling declaration (AST_STATEMENT_DECL): name = '%s', type = '%s'", st.decl.name, type_as_string(st.decl.type));
			fprintf(outp, "%s %s;", type_to_str(st.decl.type), st.decl.name);
			break;

		case AST_STATEMENT_SET:
			log_trace("Compiling set statement (AST_STATEMENT_SET): name = '%s'", st.set.name);
			fprintf(outp, "%s=", st.set.name);
			compile_expr(outp, st.set.val);
			fputc(';', outp);
			break;

		case AST_STATEMENT_CFLOW:
			log_trace("Compiling control flow statement (AST_STATEMENT_CFLOW): kind = '%s'", cflows[st.cflow.kind]);
			fprintf(outp, "%s(", cflows[st.cflow.kind]);
			compile_expr(outp, st.cflow.cond);
			fputs("){", outp);

			for (size_t i = 0; i < buffer_len(st.cflow.body); i++)
				compile_statement(outp, st.cflow.body[i]);

			fputc('}', outp);
			break;

		case AST_STATEMENT_RETURN:
			log_trace("Compiling return statement (AST_STATEMENT_RETURN)");
			fputs("return ", outp);

			compile_expr(outp, st.ret);
			fputc(';', outp);

			break;

		case AST_STATEMENT_CALL:
			compile_call(outp, st.call);
			fputc(';', outp);
			break;
	}
}

void compile_program(FILE *outp, ast_program_t program) {
	log_info("Compiling program");

	for (size_t i = 0; i < buffer_len(program.items); i++) {
		ast_tl_t item = program.items[i];

		switch (item.kind) {
			case AST_TL_INCLUDE:
				log_trace("Compiling include statement (AST_TL_INCLUDE): inc_file = %s", item.inc_file);
				fprintf(outp, "#include <%s>\n", item.inc_file);
				break;

			case AST_TL_FUNC:
				log_trace("Compiling function definition (AST_TL_FUNC): name = '%s', ret = '%s'", item.func.name, type_as_string(item.func.ret));
				
				fprintf(outp, "%s %s(", type_to_str(item.func.ret), item.func.name);

				for (size_t i = 0; i < buffer_len(item.func.args); i++) {
					ast_arg_t arg = item.func.args[i];

					fprintf(outp, "%s %s", type_to_str(arg.type), arg.name);

					if (i != buffer_len(item.func.args) - 1)
						fputc(',', outp);
				}

				if (item.func.vararg)
					fputs(",...", outp);

				fputc(')', outp);

				if (item.func.body == NULL)
					fputc(';', outp);
				else {
					fputc('{', outp);
					for (size_t i = 0; i < buffer_len(item.func.body); i++) {
						compile_statement(outp, item.func.body[i]);
					}
					fputc('}', outp);
				}
				
				break;
		}
	}
}

// Open file for output and compile input expression
void compile(ast_program_t program, char *out_path) {
	FILE *outp = stdout;

	if (out_path != NULL) {
		log_info("Opening file '%s' for output", out_path);
		outp = fopen(out_path, "w");

		if (outp == NULL) {
			int err = errno;
			error(1, "Failed to open '%s': %s", out_path, strerror(err));
		}
	}

	for (size_t i = 0; i < buffer_len(defs); i++) 
		fputs(defs[i], outp);

	compile_program(outp, program);

	log_info("Compilation complete");

	if (outp != stdout)
		fclose(outp);
	else 
		printf("\n");
}

