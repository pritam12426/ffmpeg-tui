#ifndef _TUI_H_
#define _TUI_H_

#include <ncurses.h>
#include <stdbool.h>

#include "../ffprobe_ipc.h"
#include "../project_config.h"

/* ─── Color pair indices (defined once in tui_init) ───────────────────── */
#define CP_TITLE    1   /* cyan    — title bar / selected item  */
#define CP_HIGHLIGHT 2  /* yellow  — highlights / warnings      */
#define CP_ERROR    3   /* red     — errors                     */
#define CP_OK       4   /* green   — success / confirm          */
#define CP_NORMAL   5   /* white   — normal body text           */

/* ─── Screens ─────────────────────────────────────────────────────────── */

typedef enum {
    SCREEN_HOME,        /* file info + operation list           */
    SCREEN_FILE_INFO,   /* full ffprobe metadata view           */
    SCREEN_OPERATIONS,  /* operation picker                     */
    SCREEN_SETTINGS,    /* per-operation settings form          */
    SCREEN_CONFIRM,     /* review command → confirm             */
    SCREEN_EXIT,        /* user confirmed  → print cmd, exit 0  */
    SCREEN_ABORT,       /* user quit       → exit 1             */
} Screen;

/* ─── Operations ──────────────────────────────────────────────────────── */

typedef enum {
    OP_CONVERT         = 0,
    OP_EXTRACT_AUDIO   = 1,
    OP_RESIZE          = 2,
    OP_TRIM            = 3,
    OP_COMPRESS        = 4,
    OP_MERGE           = 5,
    OP_ADD_SUBTITLES   = 6,
    OP_GIF             = 7,
    OP_THUMBNAILS      = 8,
    OP_WATERMARK       = 9,
    OP_AUDIO_ADJUST    = 10,
    OP_VIDEO_FILTERS   = 11,
    OP_COUNT,           /* keep last — used for array sizing    */
} Operation;

/* ─── Central app state ───────────────────────────────────────────────── */

typedef struct {
    Screen          screen;         /* current active screen            */

    /* parsed from CLI args */
    const char     *input_file;     /* path to the media file           */
    const char     *out_dir;        /* output directory                 */
    const char     *ffmpeg_path;    /* resolved ffmpeg binary path      */
    bool            dry_run;        /* preview only, don't execute      */

    /* ffprobe result for input_file */
    Ffprobe_result  probe;
    bool            probe_done;     /* true if probe succeeded          */

    /* user selections */
    Operation       operation;      /* chosen operation                 */
    char            ffmpeg_cmd[4096]; /* final built ffmpeg command     */

    /* terminal dimensions — updated on SIGWINCH */
    int             term_rows;
    int             term_cols;

    /* ncurses windows */
    WINDOW         *win_main;       /* fills rows 0 … term_rows-2      */
    WINDOW         *win_statusbar;  /* single row at term_rows-1        */
} App_state;

/* ─── Public API ──────────────────────────────────────────────────────── */

/*
 * tui_run — initialize ncurses, run the event loop, tear down.
 *
 * On success (user confirmed), copies the built ffmpeg command into
 * out_cmd and returns 0.  The caller prints out_cmd to stdout so the
 * shell wrapper can source it.
 *
 * Returns:
 *   0   confirmed — out_cmd populated
 *   1   aborted   — user pressed q
 *  -1   init error (terminal too small, no color support, …)
 */
int tui_run(App_state *state, char *out_cmd, size_t out_cmd_size);

/* Refresh window sizes after SIGWINCH / KEY_RESIZE. */
void tui_handle_resize(App_state *state);

#endif /* _TUI_H_ */
