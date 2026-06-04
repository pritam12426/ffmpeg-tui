#ifndef _LOG_H_
#define _LOG_H_


#define COLOR_RESET             "\x1b[0m"

#define COLOR_BLACK             "\x1b[30m"
#define COLOR_RED               "\x1b[31m"
#define COLOR_GREEN             "\x1b[32m"
#define COLOR_YELLOW            "\x1b[33m"
#define COLOR_BLUE              "\x1b[34m"
#define COLOR_MAGENTA           "\x1b[35m"
#define COLOR_CYAN              "\x1b[36m"
#define COLOR_WHITE             "\x1b[37m"

#define COLOR_BOLD_BLACK        "\x1b[1;30m"
#define COLOR_BOLD_RED          "\x1b[1;31m"
#define COLOR_BOLD_GREEN        "\x1b[1;32m"
#define COLOR_BOLD_YELLOW       "\x1b[1;33m"
#define COLOR_BOLD_BLUE         "\x1b[1;34m"
#define COLOR_BOLD_MAGENTA      "\x1b[1;35m"
#define COLOR_BOLD_CYAN         "\x1b[1;36m"
#define COLOR_BOLD_WHITE        "\x1b[1;37m"

#define COLOR_UNDERLINE_BLACK   "\x1b[4;30m"
#define COLOR_UNDERLINE_RED     "\x1b[4;31m"
#define COLOR_UNDERLINE_GREEN   "\x1b[4;32m"
#define COLOR_UNDERLINE_YELLOW  "\x1b[4;33m"
#define COLOR_UNDERLINE_BLUE    "\x1b[4;34m"
#define COLOR_UNDERLINE_MAGENTA "\x1b[4;35m"
#define COLOR_UNDERLINE_CYAN    "\x1b[4;36m"
#define COLOR_UNDERLINE_WHITE   "\x1b[4;37m"

#define COLOR_BG_BLACK          "\x1b[40m"
#define COLOR_BG_RED            "\x1b[41m"
#define COLOR_BG_GREEN          "\x1b[42m"
#define COLOR_BG_YELLOW         "\x1b[43m"
#define COLOR_BG_BLUE           "\x1b[44m"
#define COLOR_BG_MAGENTA        "\x1b[45m"
#define COLOR_BG_CYAN           "\x1b[46m"
#define COLOR_BG_WHITE          "\x1b[47m"

#define COLOR_BRIGHT_BLACK      "\x1b[90m"
#define COLOR_BRIGHT_RED        "\x1b[91m"
#define COLOR_BRIGHT_GREEN      "\x1b[92m"
#define COLOR_BRIGHT_YELLOW     "\x1b[93m"
#define COLOR_BRIGHT_BLUE       "\x1b[94m"
#define COLOR_BRIGHT_MAGENTA    "\x1b[95m"
#define COLOR_BRIGHT_CYAN       "\x1b[96m"
#define COLOR_BRIGHT_WHITE      "\x1b[97m"

#define COLOR_BG_BRIGHT_BLACK   "\x1b[100m"
#define COLOR_BG_BRIGHT_RED     "\x1b[101m"
#define COLOR_BG_BRIGHT_GREEN   "\x1b[102m"
#define COLOR_BG_BRIGHT_YELLOW  "\x1b[103m"
#define COLOR_BG_BRIGHT_BLUE    "\x1b[104m"
#define COLOR_BG_BRIGHT_MAGENTA "\x1b[105m"
#define COLOR_BG_BRIGHT_CYAN    "\x1b[106m"
#define COLOR_BG_BRIGHT_WHITE   "\x1b[107m"

#define COLOR_BOLD              "\x1b[1m"
#define COLOR_DIM               "\x1b[2m"
#define COLOR_ITALIC            "\x1b[3m"
#define COLOR_UNDERLINE         "\x1b[4m"
#define COLOR_BLINK             "\x1b[5m"
#define COLOR_REVERSE           "\x1b[7m"
#define COLOR_HIDDEN            "\x1b[8m"
#define COLOR_STRIKETHROUGH     "\x1b[9m"


/* --------------------------------------------------
 * Log levels
 * -------------------------------------------------- */
typedef enum {
	LOG_LEVEL_ERROR = 0,  /* always shown                  */
	LOG_LEVEL_WARN  = 1,
	LOG_LEVEL_INFO  = 2,  /* default                       */
	LOG_LEVEL_DEBUG = 3   /* only when explicitly enabled  */
} log_level_t;

/* g_log_level is private to log.c — use the accessors below */
void        log_set_level(log_level_t level);
log_level_t log_get_level(void);

/* --------------------------------------------------
 * Internal implementation — do not call directly.
 * Do NOT append '\n' to fmt; push_file_log() adds it.
 * -------------------------------------------------- */
void push_file_log(
	int         level,
	const char *color,
	const char *emoji,
	const char *level_name,
	const char *file,
	int         line,
	const char *func,
	const char *fmt,
	...
);

/* --------------------------------------------------
 * Public macros — do NOT add \n to format strings.
 * -------------------------------------------------- */
#define LOG_ERROR(...) \
	push_file_log(LOG_LEVEL_ERROR, COLOR_BOLD_RED,    "❗ ", "ERROR", \
	              __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_WARN(...) \
	push_file_log(LOG_LEVEL_WARN,  COLOR_BOLD_YELLOW, "⚠️ ", "WARN ", \
	              __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_INFO(...) \
	push_file_log(LOG_LEVEL_INFO,  COLOR_BOLD_GREEN,  "ℹ️ ", "INFO ", \
	              __FILE__, __LINE__, __func__, __VA_ARGS__)

#define LOG_DEBUG(...) \
	push_file_log(LOG_LEVEL_DEBUG, COLOR_BOLD_CYAN,   "🛠 ", "DEBUG", \
	              __FILE__, __LINE__, __func__, __VA_ARGS__)


#endif  // _LOG_H_
