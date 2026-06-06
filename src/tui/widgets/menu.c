#include "menu.h"

#include <ncurses.h>
#include <string.h>

#define CP_SELECTED  1   /* cyan highlight for selected row */
#define CP_NORMAL    5   /* white for normal rows           */
#define CP_DIM       5   /* dim for description text        */

void menu_init(Menu *m, const Menu_item *items, int count, int visible_rows)
{
    m->items    = items;
    m->count    = count;
    m->selected = 0;
    m->scroll   = 0;
    m->visible  = visible_rows;
}

void menu_draw(const Menu *m, WINDOW *w, int row, int col, int width)
{
    if (!m || !w || !m->items) return;

    int end = m->scroll + m->visible;
    if (end > m->count) end = m->count;

    for (int i = m->scroll; i < end; i++) {
        int y            = row + (i - m->scroll);
        bool is_selected = (i == m->selected);

        /* highlight selected row */
        if (is_selected) {
            wattron(w, COLOR_PAIR(CP_SELECTED) | A_REVERSE | A_BOLD);
            mvwhline(w, y, col, ' ', width);    /* fill row background */
        } else {
            wattron(w, COLOR_PAIR(CP_NORMAL));
        }

        /* cursor indicator */
        mvwprintw(w, y, col, " %s %-*s",
                  is_selected ? "▶" : " ",
                  width - 4,
                  m->items[i].label);

        if (is_selected) {
            wattroff(w, COLOR_PAIR(CP_SELECTED) | A_REVERSE | A_BOLD);
        } else {
            wattroff(w, COLOR_PAIR(CP_NORMAL));
        }

        /* description on the same row, right-aligned, dimmed */
        if (m->items[i].description && m->items[i].description[0] != '\0') {
            int desc_len = (int)strlen(m->items[i].description);
            int desc_col = col + width - desc_len - 2;
            if (desc_col > col + (int)strlen(m->items[i].label) + 6) {
                wattron(w, A_DIM | COLOR_PAIR(CP_DIM));
                mvwprintw(w, y, desc_col, "%s", m->items[i].description);
                wattroff(w, A_DIM | COLOR_PAIR(CP_DIM));
            }
        }
    }

    /* scroll indicator at bottom right if list overflows */
    if (m->count > m->visible) {
        int indicator_row = row + m->visible - 1;
        int pct           = (m->selected * 100) / (m->count - 1);
        wattron(w, A_DIM);
        mvwprintw(w, indicator_row, col + width - 6, " %3d%%", pct);
        wattroff(w, A_DIM);
    }
}

bool menu_input(Menu *m, int ch)
{
    switch (ch) {
    case KEY_UP:
    case 'k':
        if (m->selected > 0) {
            m->selected--;
            if (m->selected < m->scroll)
                m->scroll = m->selected;
        }
        break;

    case KEY_DOWN:
    case 'j':
        if (m->selected < m->count - 1) {
            m->selected++;
            if (m->selected >= m->scroll + m->visible)
                m->scroll = m->selected - m->visible + 1;
        }
        break;

    case KEY_HOME:
    case 'g':
        m->selected = 0;
        m->scroll   = 0;
        break;

    case KEY_END:
    case 'G':
        m->selected = m->count - 1;
        m->scroll   = m->count - m->visible;
        if (m->scroll < 0) m->scroll = 0;
        break;

    case '\n':
    case KEY_ENTER:
        return true;   /* caller should act on m->selected */
    }

    return false;
}
