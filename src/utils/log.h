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

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include <utils/ansi-fmt.h>
#include <utils/buffer.h>

// Various types of log message
typedef enum {
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_OFF
} loglevel_t;

// Formatted string for various log levels
const char *log_level_name[] = {
	[LOG_TRACE] = ANSI_FG_MAGENTA "TRACE" ANSI_RESET,
	[LOG_DEBUG] = ANSI_FG_CYAN    "DEBUG" ANSI_RESET,
	[LOG_INFO]  = ANSI_FG_GREEN   "INFO " ANSI_RESET,
	[LOG_WARN]  = ANSI_FG_YELLOW  "WARN " ANSI_RESET,
	[LOG_ERROR] = ANSI_FG_RED     "ERROR" ANSI_RESET
};

// Minimum level of importance that will be logged 
loglevel_t log_level_filter = LOG_INFO;

// Log a message with a given level, filename, line and message
void log_inner(loglevel_t level, char *filename, uint32_t line, char *fmt, ...) {
	// Only log if the importance is above the minimum level
	if (level >= log_level_filter) {
		time_t rawtime;
		struct tm *ti;

		// Get the current time
		time(&rawtime);
		ti = localtime(&rawtime);

		// Print time, log level, file and position within the file
		fprintf(
			stderr,
			ANSI_FG_WHITE "(%0*d:%0*d:%0*d) " ANSI_RESET "%s [%s:%d]: ",
			2, ti->tm_hour, 2, ti->tm_min, 2, ti->tm_sec,
			log_level_name[level],
			filename, line
		);

		va_list args;
		va_start(args, fmt);

		// Print the given message
		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");

		va_end(args);
	}
}

// Log various levels with file and position of caller attached
#define log_trace(...) log_inner(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_inner(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_inner(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_inner(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_inner(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
