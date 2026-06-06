#include "screen_settings.h"
#include "widgets/statusbar.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* ─── field types ─────────────────────────────────────────────────────── */

typedef enum { FIELD_CHOICE, FIELD_TEXT } Field_type;

typedef struct {
    const char  *label;
    Field_type   type;
    /* FIELD_CHOICE */
    const char **choices;
    int          choice_count;
    int          choice_idx;
    /* FIELD_TEXT */
    char         text[256];
} Field;

/* ─── choice option arrays ────────────────────────────────────────────── */

static const char *OPT_CONTAINER[]  = { "mp4", "mkv", "webm", "avi", "mov" };
static const char *OPT_VCODEC[]     = { "libx264", "libx265", "libvpx-vp9", "libaom-av1", "copy" };
static const char *OPT_ACODEC[]     = { "aac", "mp3", "opus", "flac", "copy" };
static const char *OPT_CRF[]        = { "18", "23", "28", "35", "51" };
static const char *OPT_PRESET[]     = { "ultrafast", "fast", "medium", "slow", "veryslow" };
static const char *OPT_RES[]        = { "3840x2160", "1920x1080", "1280x720", "854x480", "640x360" };
static const char *OPT_AUDIO_FMT[]  = { "mp3", "aac", "flac", "wav", "ogg", "opus" };
static const char *OPT_ABITRATE[]   = { "64k", "128k", "192k", "256k", "320k" };
static const char *OPT_GIF_FPS[]    = { "10", "15", "24" };
static const char *OPT_GIF_W[]      = { "320", "480", "640", "960" };
static const char *OPT_LOSSLESS[]   = { "no", "yes" };
static const char *OPT_SPEED[]      = { "0.5x", "1.0x", "2.0x" };
static const char *OPT_FLIP[]       = { "none", "horizontal", "vertical" };
static const char *OPT_ROTATE[]     = { "none", "90", "180", "270" };

/* ─── field registry ──────────────────────────────────────────────────── */

#define MAX_FIELDS 8

static Field     g_fields[MAX_FIELDS];
static int       g_field_count  = 0;
static int       g_focused      = 0;
static bool      g_initialized  = false;
static Operation g_last_op      = (Operation)-1;

#define NCHOICE(arr) ((int)(sizeof(arr)/sizeof((arr)[0])))

#define ADD_CHOICE(lbl, arr, default_idx)                    \
    do {                                                     \
        g_fields[g_field_count].label        = (lbl);        \
        g_fields[g_field_count].type         = FIELD_CHOICE; \
        g_fields[g_field_count].choices      = (arr);        \
        g_fields[g_field_count].choice_count = NCHOICE(arr); \
        g_fields[g_field_count].choice_idx   = (default_idx);\
        g_field_count++;                                     \
    } while (0)

#define ADD_TEXT(lbl, def)                                   \
    do {                                                     \
        g_fields[g_field_count].label = (lbl);               \
        g_fields[g_field_count].type  = FIELD_TEXT;          \
        strncpy(g_fields[g_field_count].text, (def),         \
                sizeof(g_fields[g_field_count].text) - 1);   \
        g_field_count++;                                     \
    } while (0)

static void init_fields(App_state *state)
{
    g_field_count = 0;
    g_focused     = 0;
    g_last_op     = state->operation;

    switch (state->operation) {

    case OP_CONVERT:
        ADD_CHOICE("Output format", OPT_CONTAINER, 0);   /* [0] */
        ADD_CHOICE("Video codec",   OPT_VCODEC,    0);   /* [1] */
        ADD_CHOICE("Audio codec",   OPT_ACODEC,    0);   /* [2] */
        ADD_CHOICE("CRF quality",   OPT_CRF,       1);   /* [3] (23) */
        ADD_CHOICE("Preset",        OPT_PRESET,    2);   /* [4] (medium) */
        break;

    case OP_EXTRACT_AUDIO:
        ADD_CHOICE("Audio format",  OPT_AUDIO_FMT, 0);   /* [0] */
        ADD_CHOICE("Bitrate",       OPT_ABITRATE,  2);   /* [1] (192k) */
        break;

    case OP_RESIZE:
        ADD_CHOICE("Resolution",    OPT_RES,    1);       /* [0] (1080p) */
        ADD_CHOICE("Video codec",   OPT_VCODEC, 0);       /* [1] */
        break;

    case OP_TRIM: {
        char end_ts[32] = "00:00:00";
        if (state->probe_done) {
            double d = state->probe.format.duration;
            int h = (int)(d/3600), m = (int)((d-h*3600)/60), s = (int)(d-h*3600-m*60);
            snprintf(end_ts, sizeof(end_ts), "%02d:%02d:%02d", h, m, s);
        }
        ADD_TEXT  ("Start time (HH:MM:SS)",  "00:00:00");  /* [0] */
        ADD_TEXT  ("End time   (HH:MM:SS)",  end_ts);      /* [1] */
        ADD_CHOICE("Lossless cut",           OPT_LOSSLESS, 0); /* [2] */
        break;
    }

    case OP_COMPRESS:
        ADD_CHOICE("CRF quality",   OPT_CRF,    2);       /* [0] (28) */
        ADD_CHOICE("Video codec",   OPT_VCODEC, 0);       /* [1] */
        ADD_CHOICE("Preset",        OPT_PRESET, 2);       /* [2] (medium) */
        break;

    case OP_GIF:
        ADD_TEXT  ("Start time (HH:MM:SS)", "00:00:00");  /* [0] */
        ADD_TEXT  ("Duration (seconds)",    "5");          /* [1] */
        ADD_CHOICE("Frame rate",            OPT_GIF_FPS, 1); /* [2] (15fps) */
        ADD_CHOICE("Output width (px)",     OPT_GIF_W,   1); /* [3] (480) */
        break;

    case OP_THUMBNAILS:
        ADD_TEXT("Timestamp (HH:MM:SS)", "00:00:05");      /* [0] */
        break;

    case OP_AUDIO_ADJUST:
        ADD_CHOICE("Audio codec",  OPT_ACODEC,  0);       /* [0] */
        ADD_CHOICE("Bitrate",      OPT_ABITRATE, 2);      /* [1] (192k) */
        break;

    case OP_VIDEO_FILTERS:
        ADD_CHOICE("Speed",   OPT_SPEED,  1);              /* [0] (1.0x) */
        ADD_CHOICE("Flip",    OPT_FLIP,   0);              /* [1] (none) */
        ADD_CHOICE("Rotate",  OPT_ROTATE, 0);              /* [2] (none) */
        break;

    case OP_MERGE:
    case OP_ADD_SUBTITLES:
    case OP_WATERMARK:
    default:
        /* placeholder — no configurable fields yet */
        ADD_TEXT("Output suffix", "_out");
        break;
    }
}

/* ─── command builder ─────────────────────────────────────────────────── */

#define C(i)  g_fields[(i)].choices[g_fields[(i)].choice_idx]
#define T(i)  g_fields[(i)].text

static void build_command(App_state *state)
{
    char  *cmd  = state->ffmpeg_cmd;
    size_t cap  = sizeof(state->ffmpeg_cmd);

    /* extract stem (filename without extension) */
    const char *inp = state->input_file;
    const char *dot = strrchr(inp, '.');
    char        stem[512] = { 0 };
    if (dot) strncpy(stem, inp, (size_t)(dot - inp));
    else     strncpy(stem, inp, sizeof(stem) - 1);

    const char *ff = state->ffmpeg_path;

    switch (state->operation) {

    case OP_CONVERT:
        snprintf(cmd, cap,
            "%s -i \"%s\""
            " -c:v %s -crf %s -preset %s"
            " -c:a %s"
            " \"%s_converted.%s\"",
            ff, inp,
            C(1), C(3), C(4),
            C(2),
            stem, C(0));
        break;

    case OP_EXTRACT_AUDIO:
        snprintf(cmd, cap,
            "%s -i \"%s\" -vn -c:a %s -b:a %s \"%s_audio.%s\"",
            ff, inp, C(0), C(1), stem, C(0));
        break;

    case OP_RESIZE:
        snprintf(cmd, cap,
            "%s -i \"%s\" -vf scale=%s -c:v %s -c:a copy \"%s_resized.mp4\"",
            ff, inp, C(0), C(1), stem);
        break;

    case OP_TRIM:
        if (strcmp(C(2), "yes") == 0)
            snprintf(cmd, cap,
                "%s -i \"%s\" -ss %s -to %s -c copy \"%s_trimmed.mp4\"",
                ff, inp, T(0), T(1), stem);
        else
            snprintf(cmd, cap,
                "%s -i \"%s\" -ss %s -to %s"
                " -c:v libx264 -c:a aac \"%s_trimmed.mp4\"",
                ff, inp, T(0), T(1), stem);
        break;

    case OP_COMPRESS:
        snprintf(cmd, cap,
            "%s -i \"%s\" -c:v %s -crf %s -preset %s -c:a copy \"%s_compressed.mp4\"",
            ff, inp, C(1), C(0), C(2), stem);
        break;

    case OP_GIF:
        snprintf(cmd, cap,
            "%s -i \"%s\" -ss %s -t %s"
            " -vf \"fps=%s,scale=%s:-1:flags=lanczos,"
            "split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse\""
            " \"%s_anim.gif\"",
            ff, inp, T(0), T(1), C(2), C(3), stem);
        break;

    case OP_THUMBNAILS:
        snprintf(cmd, cap,
            "%s -i \"%s\" -ss %s -vframes 1 \"%s_thumb.png\"",
            ff, inp, T(0), stem);
        break;

    case OP_AUDIO_ADJUST:
        snprintf(cmd, cap,
            "%s -i \"%s\" -vn -c:a %s -b:a %s \"%s_audio.%s\"",
            ff, inp, C(0), C(1), stem, C(0));
        break;

    case OP_VIDEO_FILTERS: {
        char filt[256] = { 0 };
        if (strcmp(C(0), "0.5x") == 0) strncat(filt, "setpts=2.0*PTS,",      sizeof(filt)-1);
        if (strcmp(C(0), "2.0x") == 0) strncat(filt, "setpts=0.5*PTS,",      sizeof(filt)-1);
        if (strcmp(C(1), "horizontal") == 0) strncat(filt, "hflip,",          sizeof(filt)-1);
        if (strcmp(C(1), "vertical")   == 0) strncat(filt, "vflip,",          sizeof(filt)-1);
        if (strcmp(C(2), "90")  == 0)  strncat(filt, "transpose=1,",          sizeof(filt)-1);
        if (strcmp(C(2), "180") == 0)  strncat(filt, "transpose=1,transpose=1,", sizeof(filt)-1);
        if (strcmp(C(2), "270") == 0)  strncat(filt, "transpose=2,",          sizeof(filt)-1);
        int fl = (int)strlen(filt);
        if (fl > 0 && filt[fl-1] == ',') filt[fl-1] = '\0';

        if (filt[0])
            snprintf(cmd, cap,
                "%s -i \"%s\" -vf \"%s\" -c:a copy \"%s_filtered.mp4\"",
                ff, inp, filt, stem);
        else
            snprintf(cmd, cap,
                "%s -i \"%s\" -c copy \"%s_filtered.mp4\"",
                ff, inp, stem);
        break;
    }

    default:
        snprintf(cmd, cap,
            "%s -i \"%s\" \"%s_out.mp4\"", ff, inp, stem);
        break;
    }
}

#undef C
#undef T

/* ─── draw ────────────────────────────────────────────────────────────── */

static const char *OP_NAMES[OP_COUNT] = {
    "Convert Format", "Extract Audio",  "Resize / Scale",
    "Trim / Cut",     "Compress",       "Merge / Concat",
    "Add Subtitles",  "Create GIF",     "Extract Thumbnails",
    "Watermark",      "Audio Adjust",   "Video Filters",
};

void screen_settings_draw(App_state *state)
{
    WINDOW *w    = state->win_main;
    int     rows = state->term_rows - 1;
    int     cols = state->term_cols;
    int     row  = 0;

    /* reinit when entering a (new) operation */
    if (!g_initialized || g_last_op != state->operation) {
        init_fields(state);
        g_initialized = true;
    }

    /* rebuild preview on every redraw */
    build_command(state);

    /* ── title bar ───────────────────────────────────────────────── */
    const char *op_name =
        (state->operation < OP_COUNT) ? OP_NAMES[state->operation] : "Settings";

    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    mvwhline(w, row, 0, ' ', cols);
    mvwprintw(w, row, 2, " ffpanel \u203a %s \u203a Settings ", op_name);
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    row += 2;

    /* ── field list ──────────────────────────────────────────────── */
    for (int i = 0; i < g_field_count && row < rows - 7; i++) {
        bool   focused = (i == g_focused);
        Field *f       = &g_fields[i];

        /* label */
        wattron(w, A_DIM);
        mvwprintw(w, row, 2, "%-24s", f->label);
        wattroff(w, A_DIM);

        /* value widget */
        if (focused)
            wattron(w, COLOR_PAIR(CP_TITLE) | A_REVERSE | A_BOLD);
        else
            wattron(w, COLOR_PAIR(CP_NORMAL));

        if (f->type == FIELD_CHOICE) {
            /* ◀ value ▶ */
            wprintw(w, " \u25c4 %-14s \u25ba ", f->choices[f->choice_idx]);
        } else {
            /* editable text, cursor at end when focused */
            wprintw(w, " %-20s%s ", f->text, focused ? "\u258c" : " ");
        }

        if (focused)
            wattroff(w, COLOR_PAIR(CP_TITLE) | A_REVERSE | A_BOLD);
        else
            wattroff(w, COLOR_PAIR(CP_NORMAL));

        row += 2;   /* blank line between fields */
    }

    /* ── command preview box ─────────────────────────────────────── */
    int box_top = rows - 6;
    wattron(w, A_DIM);
    mvwhline(w, box_top, 0, ACS_HLINE, cols);
    mvwprintw(w, box_top, 2, " Command Preview ");
    wattroff(w, A_DIM);

    wattron(w, COLOR_PAIR(CP_OK) | A_BOLD);
    mvwprintw(w, box_top + 1, 2, "$ ");
    wattroff(w, COLOR_PAIR(CP_OK) | A_BOLD);

    /* truncate if wider than terminal */
    char preview[512];
    strncpy(preview, state->ffmpeg_cmd, sizeof(preview) - 1);
    preview[sizeof(preview)-1] = '\0';
    if ((int)strlen(preview) > cols - 6)
        strcpy(preview + cols - 9, "\xe2\x80\xa6"); /* … */
    mvwprintw(w, box_top + 1, 4, "%s", preview);

    if (state->dry_run) {
        wattron(w, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
        mvwprintw(w, box_top + 3, 2,
            "\xe2\x9a\xa0  DRY RUN \xe2\x80\x94 command will be printed, not executed.");
        wattroff(w, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
    }

    statusbar_draw(state,
        "\u2191\u2193 Field   \u2190\u2192 Change   Enter Confirm   Esc Back");
}

/* ─── input ───────────────────────────────────────────────────────────── */

void screen_settings_input(App_state *state, int ch)
{
    if (!g_initialized) return;

    Field *f = &g_fields[g_focused];

    switch (ch) {

    /* ── field navigation ──────────────────────────────────────── */
    case KEY_UP:
    case 'k':
        if (g_focused > 0) g_focused--;
        break;

    case KEY_DOWN:
    case 'j':
    case '\t':
        if (g_focused < g_field_count - 1) g_focused++;
        break;

    /* ── cycle CHOICE left ─────────────────────────────────────── */
    case KEY_LEFT:
    case 'h':
        if (f->type == FIELD_CHOICE) {
            f->choice_idx--;
            if (f->choice_idx < 0) f->choice_idx = f->choice_count - 1;
        }
        break;

    /* ── cycle CHOICE right ────────────────────────────────────── */
    case KEY_RIGHT:
    case 'l':
        if (f->type == FIELD_CHOICE)
            f->choice_idx = (f->choice_idx + 1) % f->choice_count;
        break;

    /* ── TEXT editing ──────────────────────────────────────────── */
    case KEY_BACKSPACE:
    case 127:
        if (f->type == FIELD_TEXT) {
            int len = (int)strlen(f->text);
            if (len > 0) f->text[len - 1] = '\0';
        }
        break;

    /* ── confirm → Confirm screen ──────────────────────────────── */
    case '\n':
    case KEY_ENTER:
        build_command(state);
        state->screen = SCREEN_CONFIRM;
        break;

    /* ── back to Operations ────────────────────────────────────── */
    case 27:    /* Esc */
        g_initialized = false;
        state->screen = SCREEN_OPERATIONS;
        break;

    default:
        /* printable char → append to focused TEXT field */
        if (f->type == FIELD_TEXT && ch >= 32 && ch < 127) {
            int len = (int)strlen(f->text);
            if (len < (int)sizeof(f->text) - 1) {
                f->text[len]     = (char)ch;
                f->text[len + 1] = '\0';
            }
        }
        break;
    }
}
