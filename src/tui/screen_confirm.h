#ifndef _SCREEN_CONFIRM_H_
#define _SCREEN_CONFIRM_H_

#include "tui.h"

/*
 * Confirm screen — review the built ffmpeg command before running.
 *
 * Shows:
 *   - Input file path
 *   - Full ffmpeg command (word-wrapped)
 *   - DRY RUN notice (if --dry-run was passed)
 *   - Confirm / back prompt
 *
 * Keys:
 *   Enter / y   — confirm → SCREEN_EXIT (tui_run prints cmd to stdout)
 *   Esc   / n   — back   → SCREEN_SETTINGS
 *   q           — quit   → SCREEN_ABORT  (handled globally by tui.c)
 */

void screen_confirm_draw (App_state *state);
void screen_confirm_input(App_state *state, int ch);

#endif /* _SCREEN_CONFIRM_H_ */
