#ifndef _MENU_H_
#define _MENU_H_

#include <ncurses.h>
#include <stdbool.h>

/*
 * Menu — a simple scrollable list widget.
 *
 * The caller owns the items array (static or stack).
 * menu_input() returns true when the user pressed Enter
 * so the caller knows to act on menu.selected.
 */

#define MENU_MAX_ITEMS 32

typedef struct {
    const char *label;        /* displayed text          */
    const char *description;  /* shown on the right/below */
} Menu_item;

typedef struct {
    const Menu_item *items;   /* array of items (caller-owned) */
    int              count;   /* number of items               */
    int              selected;/* currently highlighted index   */
    int              scroll;  /* top visible item index        */
    int              visible; /* number of rows available      */
} Menu;

/* Initialize a Menu — call once before first draw */
void menu_init(Menu *m, const Menu_item *items, int count, int visible_rows);

/* Draw the menu into window w starting at (row, col), width wide */
void menu_draw(const Menu *m, WINDOW *w, int row, int col, int width);

/*
 * Handle a keypress.
 * Returns true  if Enter was pressed (item confirmed).
 * Returns false otherwise.
 */
bool menu_input(Menu *m, int ch);

#endif /* _MENU_H_ */
