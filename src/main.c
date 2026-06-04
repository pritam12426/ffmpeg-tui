#include <argp.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ffmpeg_runner.h"
#include "ffprobe_ipc.h"
#include "log.h"
#include "project_config.h"

const char *argp_program_version     = FFPANEL " " FFPANEL_VERSION;
const char *argp_program_bug_address = FFPANEL_HOMEPAGE_URL "/issues";

static char doc[]      = FFPANEL_SHORT_DESC "\v" FFPANEL_DESC;
static char args_doc[] = "<MEDIA_FILE(s)...>";

static struct argp_option options[] = {
	{ "log-level",    'L', "LEVEL", 0, "Log level (error|warn|info|debug), default: info" },
	{ "dry-run",      'n',  0,      0, "Preview the ffmpeg command without executing it"  },
	{ "out-dir",      'd', "DIR",   0, "Output directory for processed files, default: ." },
	{ "ffmpeg-path",  'f', "PATH",  0, "Path to ffmpeg binary, default: auto-detect"      },
	{ "ffprobe-path", 'p', "PATH",  0, "Path to ffprobe binary, default: auto-detect"     },
	{ 0 }
};

typedef struct {
	bool  dry_run;
	char *out_dir;
	char  ffmpeg_path[PATH_MAX];
	char  ffprobe_path[PATH_MAX];

	Log_level_t log_level;
	/* positional args */

	char **media_files;
	int    media_file_count;
} Arguments;

static Arguments G_Arguments = {
	.dry_run      = false,
	.out_dir      = ".",
	.ffmpeg_path  = { 0 },
	.ffprobe_path = { 0 },

	.log_level = LOG_LEVEL_INFO,

	.media_files      = NULL,
	.media_file_count = 0,
};

static bool validate_path(const char *path, int type);

static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
	Arguments *arguments = state->input;

	switch (key) {
	case 'L':
		if      (strcmp(arg, "error") == 0) log_set_level(LOG_LEVEL_ERROR);
		else if (strcmp(arg, "warn")  == 0) log_set_level(LOG_LEVEL_WARN);
		else if (strcmp(arg, "info")  == 0) log_set_level(LOG_LEVEL_INFO);
		else if (strcmp(arg, "debug") == 0) log_set_level(LOG_LEVEL_DEBUG);
		else argp_error(state, "unknown log level: '%s'", arg);

		arguments->log_level = log_get_level();
		break;

	case 'd': arguments->out_dir = arg;  break;
	case 'n': arguments->dry_run = true; break;

	case 'f':
		strncpy(arguments->ffmpeg_path, arg, PATH_MAX - 1);
		arguments->ffmpeg_path[PATH_MAX - 1] = '\0';
		break;

	case 'p':
		strncpy(arguments->ffprobe_path, arg, PATH_MAX - 1);
		arguments->ffprobe_path[PATH_MAX - 1] = '\0';
		break;

	case ARGP_KEY_ARGS:
		arguments->media_files      = state->argv + state->next;
		arguments->media_file_count = state->argc - state->next;
		break;

	case ARGP_KEY_END:
		/* enforce at least one media file */
		if (arguments->media_file_count == 0)
			argp_error(state,
			           "at least one MEDIA_FILE is required. "
			           "Try '%s --help' for more information.",
			           FFPANEL);

		/* validate each media file */
		// TODO: quort the file path in "<path>"
		for (int i = 0; i < arguments->media_file_count; i++)
			if (!validate_path(arguments->media_files[i], 1))
				argp_error(state, "invalid media file: '%s'", arguments->media_files[i]);

		/* validate out-dir only if explicitly overridden */
		if (strcmp(arguments->out_dir, ".") != 0)
			if (!validate_path(arguments->out_dir, 2))
				argp_error(state, "invalid output directory: '%s'", arguments->out_dir);

		/* validate ffmpeg path if explicitly set, else auto-detect */
		if (arguments->ffmpeg_path[0] != '\0') {
			if (!validate_path(arguments->ffmpeg_path, 1))
				argp_error(state, "invalid ffmpeg path: '%s'", arguments->ffmpeg_path);
		} else {
			if (!get_path_via_IPC("ffmpeg", arguments->ffmpeg_path, PATH_MAX))
				argp_error(state,
				           "ffmpeg not found in PATH. "
				           "Install ffmpeg or use --ffmpeg-path.");
		}

		/* validate ffprobe path if explicitly set, else auto-detect */
		if (arguments->ffprobe_path[0] != '\0') {
			if (!validate_path(arguments->ffprobe_path, 1))
				argp_error(state, "invalid ffprobe path: '%s'", arguments->ffprobe_path);
		} else {
			if (!get_path_via_IPC("ffprobe", arguments->ffprobe_path, PATH_MAX))
				argp_error(state,
				           "ffprobe not found in PATH. "
				           "Install ffmpeg or use --ffprobe-path.");
		}
		break;

	default:
		return ARGP_ERR_UNKNOWN;
	}

	return 0;
}

static struct argp argp = {
	.options  = options,
	.parser   = parse_opt,
	.doc      = doc,
	.args_doc = args_doc,
};

static bool validate_path(const char *path, int type /* 1 = regular file, 2 = directory */)
{
	if (path == NULL || path[0] == '\0') {
		LOG_ERROR("path is NULL or empty");
		return false;
	}

	struct stat st;
	if (stat(path, &st) != 0) {
		LOG_ERROR("path does not exist: '%s'", path);
		return false;
	}

	switch (type) {
	case 1:
		if (!S_ISREG(st.st_mode)) {
			LOG_ERROR("'%s' is not a regular file", path);
			return false;
		}
		break;
	case 2:
		if (!S_ISDIR(st.st_mode)) {
			LOG_ERROR("'%s' is not a directory", path);
			return false;
		}
		break;
	default:
		LOG_ERROR("validate_path: unknown type %d", type);
		return false;
	}

	return true;
}


int main(int argc, char *argv[])
{
	/* Parse CLI arguments first so log level is set before any logging. */
	argp_parse(&argp, argc, argv, 0, 0, &G_Arguments);


	LOG_DEBUG("ffmpeg-path  : %s", G_Arguments.ffmpeg_path);
	LOG_DEBUG("ffprobe-path : %s", G_Arguments.ffprobe_path);
	LOG_DEBUG("out-dir      : %s", G_Arguments.out_dir);
	LOG_DEBUG("dry-run      : %s", G_Arguments.dry_run ? "true" : "false");
	LOG_DEBUG("media files  : %d", G_Arguments.media_file_count);

	Ffprobe_IPC ffprobe = {
		.filepath = "\"/Users/pritam/Music/local/bandeya_rey_bandeya(small).mp3\"",
		// .filepath = "'./temp14.jpg'",
		.ffprobe_path = G_Arguments.ffprobe_path,
		.show_format   = true,
		.show_streams  = true
	};

	Ffprobe_result probe_resul = { 0 };

	run_ffprobe_IPC(&ffprobe, &probe_resul);

	int x = 5;

	ffprobe_result_free(&probe_resul);


	// puts(u.raw_json);


	return 0;
}
