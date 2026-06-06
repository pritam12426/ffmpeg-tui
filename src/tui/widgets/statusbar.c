#include "statusbar.h"

#include <ncurses.h>
#include <string.h>

#include "../../project_config.h"
#include "../tui.h"

/* Color pair indices (defined once in tui_init) */
#define CP_TITLE    1   /* cyan    — title / selected */
#define CP_HINT     2   /* yellow  — key hints        */

void statusbar_draw(App_state *state, const char *keys_hint)
{
    WINDOW *w = state->win_statusbar;
    if (!w) return;

    werase(w);
    wbkgd(w, COLOR_PAIR(CP_TITLE) | A_REVERSE);

    /* ── left: "ffpanel v0.1.0" ─────────────────────────────────── */
    wattron(w, A_BOLD | A_REVERSE);
    mvwprintw(w, 0, 1, "%s v%s", FFPANEL, FFPANEL_VERSION);
    wattroff(w, A_BOLD | A_REVERSE);

    /* ── right: caller-supplied key hints ───────────────────────── */
    if (keys_hint && keys_hint[0] != '\0') {
        int hint_len = (int)strlen(keys_hint);
        int hint_col = state->term_cols - hint_len - 1;
        if (hint_col > 0) {
            wattron(w, A_REVERSE);
            mvwprintw(w, 0, hint_col, "%s", keys_hint);
            wattroff(w, A_REVERSE);
        }
    }

    wrefresh(w);
}
