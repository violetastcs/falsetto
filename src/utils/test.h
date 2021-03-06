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
#include <stdarg.h>
#include <stdio.h>

#include <utils/log.h>

// Result of a test 
typedef struct test_result {
	bool error;
	char *error_msg;
} test_result_t;

// Test passes, no message
test_result_t test_pass() {
	return (test_result_t){ false, "" };
}

// Test failed
test_result_t test_fail(char *fmt, ...) {
	va_list argp;
	va_start(argp, fmt);

	char *buf = malloc(1024);
	vsprintf(buf, fmt, argp);

	return (test_result_t){ true, buf };
}

// Whether a test is expected to fail or not
typedef enum test_fail_t {
	TEST_EXPECT_FAIL = true,
	TEST_EXPECT_PASS = false
} test_fail_t;

typedef struct test_def {
	char *name;
	test_result_t (*func)();
	test_fail_t expect_fail;
} test_def_t;

// Execute all tests specified
int test_run(test_def_t *tests) {
	// Iterate until NULL entry encountered
	size_t i = 0;
	while (tests[i].name != 0 && tests[i].func != 0) {
		test_def_t def = tests[i];
		test_result_t result = def.func();

		if (result.error && !def.expect_fail) {
			log_error("Test `%s` failed: %s", def.name, result.error_msg);
			return 1;
		} else if (!result.error && def.expect_fail) {
			log_error("Test `%s` passed, expected fail", def.name);
			return 1;
		} else {
			log_trace("Test `%s` passed", def.name);
		}
		i++;
	}
	return 0;
}

