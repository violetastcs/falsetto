#pragma once

#include <stdint.h>
#include <stddef.h>
#include <iso646.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include <utils/log.h>
#include <utils/kv.h>

// Defines information about an application
typedef struct arg_app {
	char *name;
	char *binname;
	char *desc;
	char *usage;
	char  vmaj;
	char  vmin;
	char  vrev;
} arg_app_t;

// Defines arguments an application takes
typedef struct arg_def {
	char *long_name;
	char  short_name;
	char *value_name;
	char *description;

	bool (*check)(char *);
} arg_def_t;

// Print out a message showing commandline usage
void arg_help(arg_app_t app, arg_def_t *defs) {
	printf("%s v%d.%d.%d frontend.\n\n", app.name, app.vmaj, app.vmin, app.vrev);
	printf("Usage: %s %s\n\n", app.binname, app.usage);
	printf("%s\n\n", app.desc);

	printf("\t-h (--help)\t\tDisplays this message\n");

	size_t i = 0;
	while (defs[i].long_name != NULL) {
		arg_def_t def = defs[i];

		// If the argument takes a value specify the value
		if (def.check == NULL) 
			printf("\t-%c (--%s)\t\t%s\n", def.short_name, def.long_name, def.description);
		else 
			printf("\t-%c (--%s) [%s]\t%s\n", def.short_name, def.long_name, def.value_name, def.description);

		i++;
	}
}

char *skip_dash(char *str) {
	while (*str == '-')
		str++;

	return str;
}

// Parse commandline arguments based on definitions
bool arg_parse(arg_def_t *defs, kv_store_t *store, int argc, char **argv) {
	char *bin = basename(argv[0]);

	bool any_args = false;

	for (int i = 1; i < argc; i++) {
		// If a -- is encountered, stop parsing arguments
		if (strcmp(argv[i], "--") == 0) 
			break;

		// Return prematurely if a help message is requested
		else if (strcmp(argv[i], "--help") == 0) 
			return false;
		else if (strcmp(argv[i], "-h") == 0)
			return false;
		
		arg_def_t arg;
		bool found = false;

		// Determine whether an argument is a short or long 
		bool is_long = argv[i][0] == '-' and argv[i][1] == '-';
		bool is_short = argv[i][0] == '-' and !is_long;

		if (!is_long and !is_short) {
			printf("%s: expected option, found: %s\n", bin, argv[i]);
		}

		// Look for provided argument in definitions
		size_t j = 0;
		while (defs[j].long_name != NULL) {
			bool is_long_name = is_long and strcmp(argv[i] + 2, defs[j].long_name) == 0;
			bool is_short_name = is_short and argv[i][1] == defs[j].short_name;

			if (is_long_name or is_short_name) {
				found = true;
				arg = defs[j];
				break;
			}

			j++;
		}

		// Error if the argument isn't recognized
		if (!found) {
			printf("%s: unrecognized option: %s\n", bin, skip_dash(argv[i]));
			return false;
		}

		any_args = true;

		if (arg.check != NULL) {
			// Option takes a value. Make sure one was passed, and that it is valid
			if (++i < argc and argv[i][0] != '-') {
				if (arg.check(argv[i])) 
					kv_insert(store, arg.value_name, argv[i]);
				else {
					printf("%s: invalid value for option %s: %s\n", bin, argv[i-1], argv[i]);
					return false;
				}
			} else {
				printf("%s: option requires an argument: %s\n", bin, skip_dash(argv[i - 1]));
				return false;
			}
		} else {
			// Insert an empty string (NULL reserved for when argument not present passed)
			kv_insert(store, arg.value_name, "");
		}
	}

	return any_args;
}

// Argument takes an integer
bool arg_takes_int(char *arg) {
	bool digits = true;

	for (size_t i = 0; i < strlen(arg); i++)
		if (arg[i] < '0' or arg[i] > '9')
			digits = false;

	return digits;
}

// Argument takes any value
bool arg_takes_val(char *arg) {
	return true;
}
