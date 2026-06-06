#include "screen_confirm.h"
#include "widgets/statusbar.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

/* ─── helpers ─────────────────────────────────────────────────────────── */

/*
 * wrap_print — word-wrap str into window w starting at (row, col)
 * with the given available width.  Returns the next unused row.
 */
static int wrap_print(WINDOW *w, int row, int col, int width,
                       const char *str, int attr)
{
    int max_row = getmaxy(w) - 2;
    int len     = (int)strlen(str);
    int pos     = 0;

    wattron(w, attr);
    while (pos < len && row <= max_row) {
        int avail = width;
        int chunk = (pos + avail <= len) ? avail : (len - pos);

        /* try to break on a space */
        if (pos + chunk < len) {
            int back = chunk;
            while (back > 0 && str[pos + back] != ' ') back--;
            if (back > 0) chunk = back;
        }

        mvwprintw(w, row++, col, "%.*s", chunk, str + pos);
        pos += chunk;
        if (pos < len && str[pos] == ' ') pos++;
    }
    wattroff(w, attr);
    return row;
}

/* ─── draw ────────────────────────────────────────────────────────────── */

void screen_confirm_draw(App_state *state)
{
    WINDOW *w    = state->win_main;
    int     cols = state->term_cols;
    int     rows = state->term_rows - 1;
    int     row  = 0;

    /* ── title bar ───────────────────────────────────────────────── */
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    mvwhline(w, row, 0, ' ', cols);
    mvwprintw(w, row, 2, " ffpanel \u203a Confirm ");
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    row += 2;

    /* ── input file ──────────────────────────────────────────────── */
    wattron(w, A_DIM);
    mvwprintw(w, row, 2, "Input file:");
    wattroff(w, A_DIM);
    wattron(w, A_BOLD | COLOR_PAIR(CP_NORMAL));
    mvwprintw(w, row + 1, 4, "%s", state->input_file ? state->input_file : "(none)");
    wattroff(w, A_BOLD | COLOR_PAIR(CP_NORMAL));
    row += 3;

    /* ── command box ─────────────────────────────────────────────── */
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(w, row++, 2, "Command to run:");
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD);

    /* bordered box: single line preview */
    int box_inner_w = cols - 8;
    mvwaddch(w, row,   2, ACS_ULCORNER);
    mvwhline(w, row,   3, ACS_HLINE, box_inner_w);
    mvwaddch(w, row,   3 + box_inner_w, ACS_URCORNER);

    mvwaddch(w, row+1, 2, ACS_VLINE);
    mvwaddch(w, row+1, 3 + box_inner_w, ACS_VLINE);

    mvwaddch(w, row+2, 2, ACS_LLCORNER);
    mvwhline(w, row+2, 3, ACS_HLINE, box_inner_w);
    mvwaddch(w, row+2, 3 + box_inner_w, ACS_LRCORNER);

    /* truncated single-line inside the box */
    wattron(w, COLOR_PAIR(CP_OK) | A_BOLD);
    char preview[512];
    strncpy(preview, state->ffmpeg_cmd, sizeof(preview) - 1);
    preview[sizeof(preview)-1] = '\0';
    if ((int)strlen(preview) > box_inner_w - 2)
        strcpy(preview + box_inner_w - 5, "\xe2\x80\xa6");
    mvwprintw(w, row + 1, 4, "%s", preview);
    wattroff(w, COLOR_PAIR(CP_OK) | A_BOLD);

    row += 4;

    /* ── full command (word-wrapped) ─────────────────────────────── */
    wattron(w, A_DIM);
    mvwprintw(w, row++, 2, "Full command:");
    wattroff(w, A_DIM);

    row = wrap_print(w, row, 4, cols - 8,
                     state->ffmpeg_cmd, COLOR_PAIR(CP_NORMAL));
    row += 2;

    /* ── dry-run notice ──────────────────────────────────────────── */
    if (state->dry_run && row < rows - 4) {
        wattron(w, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
        mvwprintw(w, row++, 2,
            "\xe2\x9a\xa0  DRY RUN \xe2\x80\x94 "
            "command will be printed to stdout but NOT executed.");
        wattroff(w, COLOR_PAIR(CP_HIGHLIGHT) | A_BOLD);
        row++;
    }

    /* ── confirm prompt ──────────────────────────────────────────── */
    int prompt_row = rows - 4;
    wattron(w, A_DIM);
    mvwhline(w, prompt_row, 0, ACS_HLINE, cols);
    wattroff(w, A_DIM);

    wattron(w, COLOR_PAIR(CP_OK) | A_BOLD);
    mvwprintw(w, prompt_row + 1, 2,
        "Enter / y  \u2192  run      "
        "Esc / n  \u2192  go back      "
        "q  \u2192  quit");
    wattroff(w, COLOR_PAIR(CP_OK) | A_BOLD);

    statusbar_draw(state, "Enter/y Run   Esc/n Back   q Quit");
}

/* ─── input ───────────────────────────────────────────────────────────── */

void screen_confirm_input(App_state *state, int ch)
{
    switch (ch) {
    case '\n':
    case KEY_ENTER:
    case 'y':
    case 'Y':
        /* signal tui_run: print cmd and exit 0 */
        state->screen = SCREEN_EXIT;
        break;

    case 27:    /* Esc */
    case 'n':
    case 'N':
        state->screen = SCREEN_SETTINGS;
        break;

    default:
        break;
    }
}
