#include <stdio.h>
#include <locale.h>

#include <utils/log.h>
#include <utils/argp.h>
#include <utils/kv.h>

#include <frontend/lexer.h>
#include <frontend/parser.h>
#include <frontend/ast.h>

#include <visitors/c-gen.h>
#include <visitors/type-check.h>

// Setup CLI
arg_app_t app = {
	"Falsetto",
	NULL,
	"Compiler for a systems-level Lisp",
	"[TODO]",
	0, 1, 0
};

arg_def_t args[] = {
	{ "input",  'i',   "INPUT",  "Specify the input file",            arg_takes_val },
	{ "output", 'o',   "OUTPUT", "Specify the output file",           arg_takes_val },
	{ "loglevel", 'l', "LOG",    "Specifiy the verbosity of logging", arg_takes_val },
	{ NULL }
};

int main(int argc, char *argv[]) {
	log_level_filter = LOG_WARN;
	setlocale(LC_ALL, "");

	// Parse command line arguments
	kv_store_t arg_vals = kv_new();
	app.binname = basename(argv[0]);

	if (!arg_parse(args, &arg_vals, argc, argv)) {
		arg_help(app, args);
		return 0;
	}

	char *in_file = kv_get(&arg_vals, "INPUT");
	char *out_file = kv_get(&arg_vals, "OUTPUT");
	char *log_level = kv_get(&arg_vals, "LOG");

	if (log_level != NULL) {
		if (strcmp("trace", log_level) == 0) {
			log_level_filter = LOG_TRACE;
		} else if (strcmp("info", log_level) == 0) {
			log_level_filter = LOG_INFO;
		} else if (strcmp("warn", log_level) == 0) {
			log_level_filter = LOG_WARN;
		} else {
			error(1, "%s: invalid log level: %s", argv[0], log_level);
		}
	}

	// Tokenize, parse and compile given input 
	lexer_init_file(in_file);
	atom_t program = parse();
	ast_program_t ast = parse_program(program);
	type_check(ast);
	compile(ast, out_file);
	
	return 0;
}

