// widgets/statusbar.c

void statusbar_draw(App_state *state, const char *keys_hint)
{
    WINDOW *w = state->win_statusbar;
    werase(w);
    wbkgd(w, COLOR_PAIR(1) | A_REVERSE);

    /* left: project name + version */
    mvwprintw(w, 0, 1, FFPANEL_NAME " " FFPANEL_VERSION);

    /* center: current screen name */
    /* right: key hints passed in by each screen */
    int hint_col = state->term_cols - (int)strlen(keys_hint) - 1;
    mvwprintw(w, 0, hint_col, "%s", keys_hint);

    wrefresh(w);
}
