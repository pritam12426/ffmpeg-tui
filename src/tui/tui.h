// tui/tui.h


#ifndef _TUI_H_
#define _TUI_H_

#include <ncurses.h>
#include <stdbool.h>

#include "../ffprobe_ipc.h"
#include "../project_config.h"

/* ─── Screens ─────────────────────────────────────────────────────────── */

typedef enum {
    SCREEN_HOME,
    SCREEN_FILE_INFO,
    SCREEN_OPERATIONS,
    SCREEN_SETTINGS,
    SCREEN_CONFIRM,
    SCREEN_EXIT,        /* print command → exit 0 */
    SCREEN_ABORT,       /* user quit     → exit 1 */
} Screen;

/* ─── Operations ──────────────────────────────────────────────────────── */

typedef enum {
    OP_CONVERT,
    OP_EXTRACT_AUDIO,
    OP_RESIZE,
    OP_TRIM,
    OP_COMPRESS,
    OP_MERGE,
    OP_ADD_SUBTITLES,
    OP_GIF,
    OP_THUMBNAILS,
    OP_WATERMARK,
    OP_AUDIO_ADJUST,
    OP_VIDEO_FILTERS,
    OP_COUNT,           /* keep last — used for array sizing */
} Operation;

/* ─── Central app state ───────────────────────────────────────────────── */

typedef struct {
    /* current screen */
    Screen          screen;

    /* parsed CLI args (pointer into G_Arguments) */
    const char     *input_file;
    const char     *out_dir;
    const char     *ffmpeg_path;
    bool            dry_run;

    /* ffprobe result for the input file */
    Ffprobe_result  probe;
    bool            probe_done;

    /* user selections */
    Operation       operation;
    char            ffmpeg_cmd[4096];   /* final built command */

    /* terminal dimensions (updated on SIGWINCH) */
    int             term_rows;
    int             term_cols;

    /* ncurses windows */
    WINDOW         *win_main;
    WINDOW         *win_statusbar;
} App_state;

/* ─── Public API ──────────────────────────────────────────────────────── */

/*
 * tui_run — initialize ncurses, run the event loop, tear down.
 *
 * Returns the ffmpeg command string in out_cmd (caller's buffer),
 * or empty string if user aborted.
 *
 * Returns:
 *   0   user confirmed → out_cmd is populated
 *   1   user aborted
 *  -1   terminal too small / init error
 */
int tui_run(App_state *state, char *out_cmd, size_t out_cmd_size);

/* call on SIGWINCH to refresh dimensions */
void tui_handle_resize(App_state *state);


#endif /* _TUI_H_ */
