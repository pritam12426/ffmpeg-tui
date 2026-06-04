// tui/tui.c

#include "tui.h"
#include "screen_home.h"
#include "screen_file_info.h"
#include "screen_operations.h"
#include "screen_settings.h"
#include "screen_confirm.h"

#define MIN_COLS  80
#define MIN_ROWS  24

static int tui_init(App_state *state)
{
    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    set_escdelay(25);   /* faster Esc response */

    if (has_colors()) {
        start_color();
        use_default_colors();
        /* define your color pairs once here */
        init_pair(1, COLOR_CYAN,    -1);  /* title / selected */
        init_pair(2, COLOR_YELLOW,  -1);  /* highlights        */
        init_pair(3, COLOR_RED,     -1);  /* errors            */
        init_pair(4, COLOR_GREEN,   -1);  /* success / confirm */
        init_pair(5, COLOR_WHITE,   -1);  /* normal text       */
    }

    getmaxyx(stdscr, state->term_rows, state->term_cols);

    if (state->term_rows < MIN_ROWS || state->term_cols < MIN_COLS) {
        endwin();
        fprintf(stderr, "Terminal too small: need %dx%d, got %dx%d\n",
                MIN_COLS, MIN_ROWS, state->term_cols, state->term_rows);
        return -1;
    }

    /* main window fills everything except bottom status bar */
    state->win_main      = newwin(state->term_rows - 1, state->term_cols, 0, 0);
    state->win_statusbar = newwin(1, state->term_cols, state->term_rows - 1, 0);

    return 0;
}

static void tui_teardown(App_state *state)
{
    if (state->win_main)      delwin(state->win_main);
    if (state->win_statusbar) delwin(state->win_statusbar);
    endwin();
}

int tui_run(App_state *state, char *out_cmd, size_t out_cmd_size)
{
    if (tui_init(state) != 0) return -1;

    state->screen = SCREEN_HOME;

    while (state->screen != SCREEN_EXIT &&
           state->screen != SCREEN_ABORT) {

        /* draw current screen */
        werase(state->win_main);
        switch (state->screen) {
        case SCREEN_HOME:        screen_home_draw(state);       break;
        case SCREEN_FILE_INFO:   screen_file_info_draw(state);  break;
        case SCREEN_OPERATIONS:  screen_operations_draw(state); break;
        case SCREEN_SETTINGS:    screen_settings_draw(state);   break;
        case SCREEN_CONFIRM:     screen_confirm_draw(state);    break;
        default: break;
        }
        wrefresh(state->win_main);

        /* handle input for current screen */
        int ch = wgetch(state->win_main);

        /* global keybindings */
        if (ch == 'q' || ch == 'Q') {
            state->screen = SCREEN_ABORT;
            break;
        }
        if (ch == KEY_RESIZE) {
            tui_handle_resize(state);
            continue;
        }

        /* delegate to current screen's input handler */
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

    if (state->screen == SCREEN_EXIT && out_cmd) {
        strncpy(out_cmd, state->ffmpeg_cmd, out_cmd_size - 1);
        return 0;
    }

    return 1; /* aborted */
}

void tui_handle_resize(App_state *state)
{
    getmaxyx(stdscr, state->term_rows, state->term_cols);
    wresize(state->win_main,      state->term_rows - 1, state->term_cols);
    wresize(state->win_statusbar, 1,                    state->term_cols);
    mvwin(state->win_statusbar,   state->term_rows - 1, 0);
    clearok(stdscr, TRUE);
}
