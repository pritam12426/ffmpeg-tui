#include "ffprobe_ipc.h"

#include <cjson/cJSON.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "log.h"

#define MAX_FFPROBE_ARGS 16
#define POPEN_CMD_SIZE   4096
#define READ_CHUNK_SIZE  8192

// ─── helpers ───────────────────────────────────────────────────────────

static Stream_type parse_stream_type(const char *s)
{
	if (!s)                                 return STREAM_TYPE_UNKNOWN;
	if (strcmp(s, "video") == 0)    return STREAM_TYPE_VIDEO;
	if (strcmp(s, "audio") == 0)    return STREAM_TYPE_AUDIO;
	if (strcmp(s, "subtitle") == 0) return STREAM_TYPE_SUBTITLE;
	if (strcmp(s, "data") == 0)     return STREAM_TYPE_DATA;

	return STREAM_TYPE_UNKNOWN;
}

// safely copy a cJSON string field into a fixed buffer, silently truncates
#define CJSON_STR(obj, key, dst)                                       \
	do {                                                               \
		cJSON *_item = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
		if (cJSON_IsString(_item) && _item->valuestring)               \
			strncpy((dst), _item->valuestring, sizeof(dst) - 1);       \
	} while (0)

// safely read a cJSON int field
#define CJSON_INT(obj, key, dst)                                       \
	do {                                                               \
		cJSON *_item = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
		if (cJSON_IsNumber(_item))                                     \
			(dst) = (int) _item->valuedouble;                          \
	} while (0)

// safely read a cJSON double field
#define CJSON_DBL(obj, key, dst)                                       \
	do {                                                               \
		cJSON *_item = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
		if (cJSON_IsNumber(_item))                                     \
			(dst) = _item->valuedouble;                                \
		else if (cJSON_IsString(_item) && _item->valuestring)          \
			(dst) = atof(_item->valuestring);                          \
	} while (0)

// safely read a cJSON int64 field (ffprobe emits these as strings)
#define CJSON_I64(obj, key, dst)                                       \
	do {                                                               \
		cJSON *_item = cJSON_GetObjectItemCaseSensitive((obj), (key)); \
		if (cJSON_IsString(_item) && _item->valuestring)               \
			(dst) = (int64_t) atoll(_item->valuestring);               \
		else if (cJSON_IsNumber(_item))                                \
			(dst) = (int64_t) _item->valuedouble;                      \
	} while (0)

// ─── JSON parsers ──────────────────────────────────────────────────────

static void parse_stream(const cJSON *js, Stream_info *s)
{
	/* zero-init so unset fields have safe defaults */
	memset(s, 0, sizeof(*s));
	s->bit_rate = -1;
	s->duration = -1.0;

	CJSON_INT(js, "index", s->index);
	CJSON_STR(js, "codec_name", s->codec_name);
	CJSON_STR(js, "codec_long_name", s->codec_long_name);
	CJSON_STR(js, "profile", s->profile);
	CJSON_I64(js, "bit_rate", s->bit_rate);
	CJSON_DBL(js, "duration", s->duration);
	CJSON_I64(js, "nb_frames", s->nb_frames);

	// stream type
	cJSON *ct = cJSON_GetObjectItemCaseSensitive(js, "codec_type");
	s->type   = parse_stream_type(cJSON_IsString(ct) ? ct->valuestring : NULL);

	// video fields
	CJSON_INT(js, "width", s->width);
	CJSON_INT(js, "height", s->height);
	CJSON_STR(js, "r_frame_rate", s->r_frame_rate);
	CJSON_STR(js, "pix_fmt", s->pix_fmt);

	// audio fields
	CJSON_INT(js, "channels", s->channels);
	CJSON_STR(js, "channel_layout", s->channel_layout);
	CJSON_STR(js, "sample_fmt", s->sample_fmt);

	// sample_rate comes as a string in ffprobe JSON e.g. "48000"
	cJSON *sr = cJSON_GetObjectItemCaseSensitive(js, "sample_rate");
	if (cJSON_IsString(sr) && sr->valuestring) s->sample_rate = atoi(sr->valuestring);

	// language lives inside tags sub-object
	cJSON *tags = cJSON_GetObjectItemCaseSensitive(js, "tags");
	if (cJSON_IsObject(tags)) CJSON_STR(tags, "language", s->language);
}

static void parse_format(const cJSON *js, Format_info *f)
{
	memset(f, 0, sizeof(*f));

	CJSON_STR(js, "filename", f->filename);
	CJSON_STR(js, "format_name", f->format_name);
	CJSON_STR(js, "format_long_name", f->format_long_name);
	CJSON_INT(js, "nb_streams", f->nb_streams);
	CJSON_DBL(js, "duration", f->duration);
	CJSON_I64(js, "size", f->size);
	CJSON_I64(js, "bit_rate", f->bit_rate);

	// metadata tags sub-object
	cJSON *tags = cJSON_GetObjectItemCaseSensitive(js, "tags");
	if (cJSON_IsObject(tags)) {
		CJSON_STR(tags, "title", f->title);
		CJSON_STR(tags, "artist", f->artist);
		CJSON_STR(tags, "encoder", f->encoder);
	}
}

// ─── public API ────────────────────────────────────────────────────────

int run_ffprobe_IPC(const Ffprobe_IPC *cfg, Ffprobe_result *out)
{
	if (cfg == NULL || out == NULL) {
		LOG_ERROR("run_ffprobe_IPC: NULL argument");
		return -1;
	}
	if (cfg->filepath == NULL || cfg->filepath[0] == '\0') {
		LOG_ERROR("run_ffprobe_IPC: filepath is NULL or empty");
		return -1;
	}
	if (cfg->ffprobe_path == NULL || cfg->ffprobe_path[0] == '\0') {
		LOG_ERROR("run_ffprobe_IPC: ffprobe_path is NULL or empty");
		return -1;
	}

	memset(out, 0, sizeof(*out));

	LOG_INFO("Preparing %s for metadata snapshot.", cfg->filepath);

	// ── build popen command string ────────────────────────────────────
	char   cmd[POPEN_CMD_SIZE] = { 0 };
	size_t pos                 = 0;

	const char *argv[MAX_FFPROBE_ARGS];
	int         argc = 0;

	argv[argc++] = cfg->ffprobe_path;
	argv[argc++] = "-v";
	argv[argc++] = "quiet";
	argv[argc++] = "-print_format";
	argv[argc++] = "json";

	if (cfg->show_streams)   argv[argc++] = "-show_streams";
	if (cfg->show_format)    argv[argc++] = "-show_format";
	// if (cfg->show_chapters)  argv[argc++] = "-show_chapters";
	// if (cfg->show_packets)   argv[argc++] = "-show_packets";

	argv[argc++] = cfg->filepath;
	argv[argc]   = NULL;

	for (int i = 0; i < argc && pos < sizeof(cmd) - 2; i++) {
		if (i > 0) cmd[pos++] = ' ';
		size_t len = strlen(argv[i]);
		if (pos + len >= sizeof(cmd) - 1) {
			LOG_ERROR("command string too long");
			return -1;
		}
		memcpy(cmd + pos, argv[i], len);
		pos += len;
	}
	cmd[pos] = '\0';

	LOG_DEBUG("Executing: %s", cmd);

	// ── spawn ffprobe and read output ─────────────────────────────────
	FILE *fp = popen(cmd, "r");
	if (!fp) {
		LOG_ERROR("popen failed");
		return -1;
	}

	/* read entire output into a dynamically growing buffer */
	size_t capacity = READ_CHUNK_SIZE;
	size_t length   = 0;
	char  *buf      = malloc(capacity);  // TODO: check that if we can you open file directly with CJSON
	if (!buf) {
		LOG_ERROR("malloc failed");
		pclose(fp);
		return -1;
	}

	char chunk[READ_CHUNK_SIZE];
	while (fgets(chunk, sizeof(chunk), fp)) {
		size_t chunk_len = strlen(chunk);
		if (length + chunk_len + 1 > capacity) {
			capacity  *= 2;
			char *tmp  = realloc(buf, capacity);
			if (!tmp) {
				LOG_ERROR("realloc failed");
				free(buf);
				pclose(fp);
				return -1;
			}
			buf = tmp;
		}
		memcpy(buf + length, chunk, chunk_len);
		length += chunk_len;
	}
	buf[length] = '\0';

	int exit_code = pclose(fp);
	if (exit_code != 0) {
		LOG_ERROR("ffprobe exited with code %d", exit_code);
		free(buf);
		return exit_code;
	}

	if (length == 0) {
		LOG_ERROR("ffprobe returned empty output");
		free(buf);
		return -2;
	}

	LOG_DEBUG("ffprobe output: %zu bytes", length);

	// ── parse JSON ────────────────────────────────────────────────────
	cJSON *root = cJSON_Parse(buf);
	if (!root) {
		const char *err = cJSON_GetErrorPtr();
		LOG_ERROR("JSON parse error near: %s", err ? err : "unknown");
		free(buf);
		return -2;
	}

	// format
	cJSON *fmt = cJSON_GetObjectItemCaseSensitive(root, "format");
	if (cJSON_IsObject(fmt))
		parse_format(fmt, &out->format);

	//  streams
	cJSON *streams = cJSON_GetObjectItemCaseSensitive(root, "streams");
	if (cJSON_IsArray(streams)) {
		int n             = cJSON_GetArraySize(streams);
		out->stream_count = (n > MAX_STREAMS) ? MAX_STREAMS : n;
		for (int i = 0; i < out->stream_count; i++)
			parse_stream(cJSON_GetArrayItem(streams, i), &out->streams[i]);
	}

	cJSON_Delete(root);

	// hand raw JSON to caller
	out->raw_json = buf; /* caller must free via ffprobe_result_free() */

	LOG_DEBUG("Parsed %d stream(s) for %s", out->stream_count, cfg->filepath);
	return 0;
}

void ffprobe_result_free(Ffprobe_result *out)
{
	// TODO: No need of raw json string
	if (!out) return;
	free(out->raw_json);
	out->raw_json = NULL;
}

const Stream_info *ffprobe_first_stream(const Ffprobe_result *out, Stream_type type)
{
	if (!out) return NULL;
	for (int i = 0; i < out->stream_count; i++)
		if (out->streams[i].type == type) return &out->streams[i];
	return NULL;
}
