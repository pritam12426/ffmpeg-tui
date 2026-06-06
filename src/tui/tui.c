#include "tui.h"

#include <stdio.h>
#include <string.h>

#include "screen_home.h"
#include "screen_file_info.h"
#include "screen_operations.h"
#include "screen_settings.h"
#include "screen_confirm.h"

#define MIN_COLS 80
#define MIN_ROWS 24

/* ─── init / teardown ─────────────────────────────────────────────────── */

static int tui_init(App_state *state)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    set_escdelay(25);   /* snappier Esc response */

    if (!has_colors()) {
        endwin();
        fprintf(stderr, "ffpanel: terminal does not support colors.\n");
        return -1;
    }

    start_color();
    use_default_colors();   /* -1 = terminal default background */

    init_pair(CP_TITLE,     COLOR_CYAN,    -1);
    init_pair(CP_HIGHLIGHT, COLOR_YELLOW,  -1);
    init_pair(CP_ERROR,     COLOR_RED,     -1);
    init_pair(CP_OK,        COLOR_GREEN,   -1);
    init_pair(CP_NORMAL,    COLOR_WHITE,   -1);

    getmaxyx(stdscr, state->term_rows, state->term_cols);

    if (state->term_rows < MIN_ROWS || state->term_cols < MIN_COLS) {
        endwin();
        fprintf(stderr,
                "ffpanel: terminal too small — need %dx%d, got %dx%d.\n",
                MIN_COLS, MIN_ROWS,
                state->term_cols, state->term_rows);
        return -1;
    }

    /* main window: all rows except the bottom statusbar row */
    state->win_main = newwin(state->term_rows - 1, state->term_cols, 0, 0);
    if (!state->win_main) { endwin(); return -1; }

    /* statusbar: single row at the very bottom */
    state->win_statusbar = newwin(1, state->term_cols, state->term_rows - 1, 0);
    if (!state->win_statusbar) { delwin(state->win_main); endwin(); return -1; }

    keypad(state->win_main, TRUE);

    return 0;
}

static void tui_teardown(App_state *state)
{
    if (state->win_main)      { delwin(state->win_main);      state->win_main      = NULL; }
    if (state->win_statusbar) { delwin(state->win_statusbar); state->win_statusbar = NULL; }
    endwin();
}

/* ─── resize ──────────────────────────────────────────────────────────── */

void tui_handle_resize(App_state *state)
{
    getmaxyx(stdscr, state->term_rows, state->term_cols);

    wresize(state->win_main,      state->term_rows - 1, state->term_cols);
    wresize(state->win_statusbar, 1,                    state->term_cols);
    mvwin(state->win_statusbar,   state->term_rows - 1, 0);

    clearok(stdscr, TRUE);
    clearok(state->win_main, TRUE);
}

/* ─── main event loop ─────────────────────────────────────────────────── */

int tui_run(App_state *state, char *out_cmd, size_t out_cmd_size)
{
    if (tui_init(state) != 0) return -1;

    state->screen = SCREEN_HOME;

    while (state->screen != SCREEN_EXIT &&
           state->screen != SCREEN_ABORT)
    {
        /* draw ---------------------------------------------------------- */
        werase(state->win_main);

        switch (state->screen) {
        case SCREEN_HOME:       screen_home_draw(state);       break;
        case SCREEN_FILE_INFO:  screen_file_info_draw(state);  break;
        case SCREEN_OPERATIONS: screen_operations_draw(state); break;
        case SCREEN_SETTINGS:   screen_settings_draw(state);   break;
        case SCREEN_CONFIRM:    screen_confirm_draw(state);    break;
        default: break;
        }

        wrefresh(state->win_main);

        /* input --------------------------------------------------------- */
        int ch = wgetch(state->win_main);

        /* global: terminal resize */
        if (ch == KEY_RESIZE) {
            tui_handle_resize(state);
            continue;
        }

        /* global: quit from anywhere */
        if (ch == 'q' || ch == 'Q') {
            state->screen = SCREEN_ABORT;
            break;
        }

        /* delegate to current screen */
        switch (state->screen) {
        case SCREEN_HOME:       screen_home_input(state, ch);       break;
        case SCREEN_FILE_INFO:  screen_file_info_input(state, ch);  break;
        case SCREEN_OPERATIONS: screen_operations_input(state, ch); break;
        case SCREEN_SETTINGS:   screen_settings_input(state, ch);   break;
        case SCREEN_CONFIRM:    screen_confirm_input(state, ch);    break;
        default: break;
        }
    }

    tui_teardown(state);

    if (state->screen == SCREEN_EXIT && out_cmd && out_cmd_size > 0) {
        strncpy(out_cmd, state->ffmpeg_cmd, out_cmd_size - 1);
        out_cmd[out_cmd_size - 1] = '\0';
        return 0;
    }

    return 1; /* aborted */
}
