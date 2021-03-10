#include <stdio.h>
#include <locale.h>

#include <utils/log.h>
#include <utils/argp.h>
#include <utils/kv.h>

#include <frontend/lexer.h>
#include <frontend/parser.h>

#include <backend/c-gen.h>

arg_app_t app = {
	"Falsetto",
	NULL,
	"Compiler for a systems-level Lisp",
	"[TODO]",
	0, 1, 0
};

arg_def_t args[] = {
	{ "input",  'i', "INPUT",  "Specify the input file",  arg_takes_val },
	{ "output", 'o', "OUTPUT", "Specify the output file", arg_takes_val },
	{ NULL }
};

int main(int argc, char *argv[]) {
	log_level_filter = LOG_TRACE;
	setlocale(LC_ALL, "");

	kv_store_t arg_vals = kv_new();
	app.binname = basename(argv[0]);

	if (!arg_parse(args, &arg_vals, argc, argv)) {
		arg_help(app, args);
		return 0;
	}

	char *in_file = kv_get(&arg_vals, "INPUT");
	char *out_file = kv_get(&arg_vals, "OUTPUT");

	lexer_init_file(in_file);
	atom_t program = parse();
	compile(program, out_file);
	
	return 0;
}

