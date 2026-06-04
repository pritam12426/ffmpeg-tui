#include "log.h"

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>  /* isatty(), fileno() */

static log_level_t g_log_level = LOG_LEVEL_INFO;

void log_set_level(log_level_t level)
{
	g_log_level = level;
}

log_level_t log_get_level(void)
{
	return g_log_level;
}

void push_file_log(
	int         level,
	const char *color,
	const char *emoji,
	const char *level_name,
	const char *file,
	int         line,
	const char *func,
	const char *fmt,
	...)
{
	/* Suppress levels above the current threshold.
	 * ERROR=0 is always shown; DEBUG=3 only when g_log_level==DEBUG. */
	if (level > (int)g_log_level)
		return;

	/* Strip colors when stderr is not a terminal (e.g. piped to a file). */
	int         use_color = isatty(fileno(stderr));
	const char *c         = use_color ? color        : "";
	const char *reset     = use_color ? COLOR_RESET  : "";
	const char *dim       = use_color ? COLOR_DIM    : "";

	fprintf(stderr,
		"%s [%s%-5s%s] %s[%s:%d:%s]%s ",
		emoji,
		c, level_name, reset,
		dim, file, line, func, reset);

	va_list args;
	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);

	fputc('\n', stderr);
}
