#pragma once

#include <stdio.h>
#include <time.h>
#include <stdarg.h>

#include <utils/ansi-fmt.h>
#include <utils/buffer.h>

typedef enum {
	LOG_TRACE,
	LOG_DEBUG,
	LOG_INFO,
	LOG_WARN,
	LOG_ERROR,
	LOG_OFF
} loglevel_t;

const char *log_level_name[] = {
	[LOG_TRACE] = ANSI_FG_MAGENTA "TRACE" ANSI_RESET,
	[LOG_DEBUG] = ANSI_FG_CYAN    "DEBUG" ANSI_RESET,
	[LOG_INFO]  = ANSI_FG_GREEN   "INFO " ANSI_RESET,
	[LOG_WARN]  = ANSI_FG_YELLOW  "WARN " ANSI_RESET,
	[LOG_ERROR] = ANSI_FG_RED     "ERROR" ANSI_RESET
};

loglevel_t log_level_filter = LOG_INFO;

void log_inner(loglevel_t level, char *filename, uint32_t line, char *fmt, ...) {
	if (level >= log_level_filter) {
		time_t rawtime;
		struct tm *ti;

		time(&rawtime);
		ti = localtime(&rawtime);

		fprintf(
			stderr,
			ANSI_FG_WHITE "(%0*d:%0*d:%0*d) " ANSI_RESET "%s [%s:%d]: ",
			2, ti->tm_hour, 2, ti->tm_min, 2, ti->tm_sec,
			log_level_name[level],
			filename, line
		);

		va_list args;
		va_start(args, fmt);

		vfprintf(stderr, fmt, args);
		fprintf(stderr, "\n");

		va_end(args);
	}
}

#define log_trace(...) log_inner(LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define log_debug(...) log_inner(LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_inner(LOG_INFO,  __FILE__, __LINE__, __VA_ARGS__)
#define log_warn(...)  log_inner(LOG_WARN,  __FILE__, __LINE__, __VA_ARGS__)
#define log_error(...) log_inner(LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
