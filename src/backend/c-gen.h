#pragma once

#include <stddef.h>
#include <errno.h>
#include <stdio.h>

#include <utils/misc.h>
#include <utils/buffer.h>

#include <frontend/parser.h>

void compile_args(FILE *outp, atom_t args) {
	fputc('(', outp);

	for (size_t i = 0; i < buffer_len(args.expr); i++) {
		atom_t arg = args.expr[i];

		if (buffer_len(arg.expr) != 2)
			error(1, "Malformed argument");

		fputs(arg.expr[1].symbol_val, outp);
		fputc(' ', outp);
		fputs(arg.expr[0].symbol_val, outp);

		if (i + 1 != buffer_len(args.expr))
			fputc(',', outp);
	}

	fputc(')', outp);
}

void compile_expr(FILE *outp, atom_t expr) {
	fputc('(', outp);

	switch (expr.kind) {
		case ATOM_INTEGER:
			fprintf(outp, "%ld", expr.integer_val);
			break;
		case ATOM_FLOAT:
			fprintf(outp, "%f", expr.float_val);
			break;
		case ATOM_SYMBOL:
			fprintf(outp, "%s", expr.symbol_val);
			break;
		case ATOM_STRING:
			fprintf(outp, "\"%s\"", expr.string_val);
			break;
		case ATOM_EXPR:
			if (!is_symbol(expr.expr[0], NULL))
				error(1, "Expected symbol");

			char *operator = expr.expr[0].symbol_val;

			if (strcmp(operator, "=") == 0) {
				compile_expr(outp, expr.expr[1]);
				fputs("==", outp);
				compile_expr(outp, expr.expr[2]);
			} else {
				fprintf(outp, "%s(", operator);

				for (size_t i = 1; i < buffer_len(expr.expr); i++) {
					compile_expr(outp, expr.expr[i]);

					if (i == buffer_len(expr.expr) - 2)
						fputc(',', outp);
				}

				fputc(')', outp);
			}
			
			break;
		default:
			TODO("Implement expression handling");
	}

	fputc(')', outp);
}

void compile_body(FILE *outp, atom_t body);

void compile_statement(FILE *outp, atom_t expr) {
	if (is_symbol(expr.expr[0], "return")) {
		fprintf(outp, "return ");
		compile_expr(outp, expr.expr[1]);
		fputc(';', outp);
	} else if (is_symbol(expr.expr[0], "if")) {
		fprintf(outp, "if(");
		compile_expr(outp, expr.expr[1]);
		fputc(')', outp);
		compile_body(outp, expr.expr[2]);
	} else if (is_symbol(expr.expr[0], NULL)) {
		fprintf(outp, "%s(", expr.expr[0].symbol_val);

		for (size_t i = 1; i < buffer_len(expr.expr); i++) {
			compile_expr(outp, expr.expr[i]);

			if (i == buffer_len(expr.expr) - 2)
				fputc(',', outp);
		}

		fprintf(outp, ");");
	}
}

void compile_body(FILE *outp, atom_t body) {
	fputc('{', outp);

	for (size_t i = 0; i < buffer_len(body.expr); i++) {
		compile_statement(outp, body.expr[i]);
	}

	fputc('}', outp);
}

void compile_func(FILE *outp, atom_t func) {
	assert(is_symbol(func.expr[0], "func"));

	if (buffer_len(func.expr) != 5)
		error(1, "Malformed function definition");

	atom_t name_a = func.expr[1];
	atom_t args_a = func.expr[2];
	atom_t ret_a = func.expr[3];
	atom_t body_a = func.expr[4];

	if (!is_symbol(name_a, NULL))
		error(1, "Function name must be symbol");
	if (!is_symbol(ret_a, NULL))
		error(1, "Return type must be symbol");

	fputs(ret_a.symbol_val, outp);
	fputc(' ', outp);
	fputs(name_a.symbol_val, outp);

	compile_args(outp, args_a);

	compile_body(outp, body_a);
}

void compile_include(FILE *outp, atom_t inc) {
	fprintf(outp, "#include <%s>\n", inc.expr[1].string_val);
}

void compile_tl(FILE *outp, atom_t program) {
	assert(program.kind == ATOM_EXPR);

	for (size_t i = 0; i < buffer_len(program.expr); i++) {
		atom_t expr = program.expr[i];

		if (buffer_len(expr.expr) == 0)
			continue;

		if (is_symbol(expr.expr[0], "func")) 
			compile_func(outp, expr);
		else if (is_symbol(expr.expr[0], "include"))
			compile_include(outp, expr);
		else 
			error(1, "Unknown expression");
	}
}

void compile(atom_t program, char *out_path) {
	FILE *outp = stdout;

	if (out_path != NULL) {
		outp = fopen(out_path, "w");

		if (outp == NULL) {
			int err = errno;
			error(1, "Failed to open '%s': %s", out_path, strerror(err));
		}
	}

	compile_tl(outp, program);

	if (outp != stdout)
		fclose(outp);
}
