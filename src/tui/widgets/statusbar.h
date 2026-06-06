#ifndef _STATUSBAR_H_
#define _STATUSBAR_H_

#include "../tui.h"

/*
 * statusbar_draw — render the bottom status bar.
 *
 * keys_hint : right-aligned key hint string, e.g. "↑↓ Navigate  Enter Select  q Quit"
 *             each screen passes its own hint.
 */
void statusbar_draw(App_state *state, const char *keys_hint);

#endif /* _STATUSBAR_H_ */
