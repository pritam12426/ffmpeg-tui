#ifndef _SCREEN_OPERATIONS_H_
#define _SCREEN_OPERATIONS_H_

#include "tui.h"

/*
 * Operations screen — choose which ffmpeg operation to perform.
 *
 * Layout (split-pane):
 *   Left 60%  : scrollable menu of all 12 operations
 *   Right 40% : description of the currently highlighted operation
 *
 * Keys:
 *   ↑ ↓ / j k  — navigate
 *   Enter       — select → go to Settings
 *   i           — go to File Info
 *   Esc / h     — go back to Home
 *   q           — quit
 */

void screen_operations_draw (App_state *state);
void screen_operations_input(App_state *state, int ch);

#endif /* _SCREEN_OPERATIONS_H_ */
