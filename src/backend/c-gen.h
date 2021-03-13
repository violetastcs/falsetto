#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdio.h>

#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/ast.h>

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
			fputc(expr.bool_val ? 1 : 0, outp);
			break;

		case AST_EXPR_BINOP:
			fputc('(', outp);

			compile_expr(outp, *expr.binop.args[0]);
			fputs(binops[expr.binop.kind], outp);
			compile_expr(outp, *expr.binop.args[1]);
			
			fputc(')', outp);
			break;

		case AST_EXPR_UNIOP:
			fputc('(', outp);

			fputs(unops[expr.unop.kind], outp);
			compile_expr(outp, *expr.unop.arg);

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
			fprintf(outp, "%s %s;", type_to_str(st.decl.type), st.decl.name);
			break;

		case AST_STATEMENT_SET:
			fprintf(outp, "%s=", st.set.name);
			compile_expr(outp, st.set.val);
			fputc(';', outp);
			break;

		case AST_STATEMENT_CFLOW:
			fprintf(outp, "%s(", cflows[st.cflow.kind]);
			compile_expr(outp, st.cflow.cond);
			fputs("){", outp);

			for (size_t i = 0; i < buffer_len(st.cflow.body); i++)
				compile_statement(outp, st.cflow.body[i]);

			fputc('}', outp);
			break;

		case AST_STATEMENT_RETURN:
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
	for (size_t i = 0; i < buffer_len(program.items); i++) {
		ast_tl_t item = program.items[i];

		switch (item.kind) {
			case AST_TL_INCLUDE:
				fprintf(outp, "#include <%s>\n", item.inc_file);
				break;

			case AST_TL_FUNC:
				fprintf(outp, "%s %s(", type_to_str(item.func.ret), item.func.name);

				for (size_t i = 0; i < buffer_len(item.func.args); i++) {
					ast_arg_t arg = item.func.args[i];

					if (type_cmp(arg.type, type_kind(TYPE_VARARG))) {
						fputs("...", outp);
						break;
					} else 
						fprintf(outp, "%s %s", type_to_str(arg.type), arg.name);

					if (i != buffer_len(item.func.args) - 1)
						fputc(',', outp);
				}

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
		outp = fopen(out_path, "w");

		if (outp == NULL) {
			int err = errno;
			error(1, "Failed to open '%s': %s", out_path, strerror(err));
		}
	}

	compile_program(outp, program);

	if (outp != stdout)
		fclose(outp);
}

