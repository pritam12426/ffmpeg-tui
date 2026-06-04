#ifndef _FFPRROBE_RUNNER_
#define _FFPRROBE_RUNNER_


#include <stdbool.h>
#include <stdint.h>
#include <string.h>


/* ─── Stream info (one per track) ─────────────────────────────────────── */

typedef enum {
	STREAM_TYPE_VIDEO,
	STREAM_TYPE_AUDIO,
	STREAM_TYPE_SUBTITLE,
	STREAM_TYPE_DATA,
	STREAM_TYPE_UNKNOWN,
} Stream_type;


typedef struct {
	int         index;
	Stream_type type;

	char        codec_name[32];       /* "h264", "aac", "subrip" … */
	char        codec_long_name[64];
	char        profile[32];          /* "High", "Main" … */

	/* video-only */
	int         width;
	int         height;
	char        r_frame_rate[16];     /* "30/1", "24000/1001" */
	char        pix_fmt[16];          /* "yuv420p" */
	int64_t     nb_frames;

	/* audio-only */
	int         sample_rate;          /* Hz */
	int         channels;
	char        channel_layout[16];   /* "stereo", "5.1" */
	char        sample_fmt[16];       /* "fltp", "s16" */

	/* common */
	int64_t     bit_rate;             /* bits/s, -1 if unknown */
	double      duration;             /* seconds, -1 if unknown */
	char        language[8];          /* ISO 639, e.g. "eng" */
} Stream_info;

/* ─── Format / container info ─────────────────────────────────────────── */

typedef struct {
	char        filename[512];
	char        format_name[64];      /* "mov,mp4,m4a,…" */
	char        format_long_name[128];
	int         nb_streams;
	double      duration;             /* seconds */
	int64_t     size;                 /* bytes */
	int64_t     bit_rate;             /* bits/s */

	/* common metadata tags */
	char        title[128];
	char        artist[128];
	char        encoder[128];
} Format_info;

/* ─── Top-level result returned by run_ffprobe_IPC ────────────────────── */

#define MAX_STREAMS 8

typedef struct {
	Format_info  format;
	Stream_info  streams[MAX_STREAMS];
	int          stream_count;

	/* raw JSON kept in case caller wants to dig deeper */
	// TODO: No need of this shit
	char        *raw_json;            /* heap-allocated, caller must free() */
} Ffprobe_result;

/* ─── IPC config passed in ────────────────────────────────────────────── */

typedef struct {
	const char *filepath;             /* path to the media file (required)  */

	bool        show_streams;         /* -show_streams                       */
	bool        show_format;          /* -show_format                        */
	// bool        show_chapters;        /* -show_chapters                      */
	// bool        show_packets;         /* -show_packets  (rarely needed)      */

	const char *ffprobe_path;         /* NULL = auto-detect from $PATH       */
} Ffprobe_IPC;

/* ─── API ─────────────────────────────────────────────────────────────── */

/*
 * run_ffprobe_IPC — spawn ffprobe via fork+execvp, capture JSON output
 * over a pipe, parse it, and fill *out.
 *
 * Parameters:
 *   cfg  : ffprobe options and target file
 *   out  : result struct to populate (must not be NULL)
 *
 * Returns:
 *    0   success  — *out is populated, out->raw_json heap-allocated
 *   -1   fork / exec / pipe error  (check errno)
 *   -2   ffprobe JSON parse error
 *   >0   ffprobe exit code on ffprobe failure
 *
 * The caller is responsible for calling ffprobe_result_free(out)
 * after use to release out->raw_json.
 */
int run_ffprobe_IPC(const Ffprobe_IPC *cfg, Ffprobe_result *out);

/*
 * ffprobe_result_free — release heap memory inside a Ffprobe_result.
 * Does NOT free the struct itself (stack-allocated callers).
 */
void ffprobe_result_free(Ffprobe_result *out);

/* convenience: find the first stream of a given type, or NULL */
const Stream_info *ffprobe_first_stream(const Ffprobe_result *out,
                                         Stream_type type);



#endif /* _FFPRROBE_RUNNER_ */
