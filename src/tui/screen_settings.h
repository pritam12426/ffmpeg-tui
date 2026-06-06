#ifndef _SCREEN_SETTINGS_H_
#define _SCREEN_SETTINGS_H_

#include "tui.h"

/*
 * Settings screen — per-operation parameter form.
 *
 * Each operation has its own set of fields:
 *   FIELD_CHOICE  — cycle left/right through a fixed list of options
 *   FIELD_TEXT    — free text edited in-place (backspace supported)
 *
 * The screen rebuilds the ffmpeg command in real time as fields change,
 * showing a command preview at the bottom before the user confirms.
 *
 * Keys:
 *   ↑ ↓ / j k  — move between fields
 *   ← →        — cycle CHOICE values
 *   Backspace   — delete last char in TEXT field
 *   any char    — append to TEXT field
 *   Enter       — confirm → go to Confirm screen
 *   Esc         — go back to Operations
 *   q           — quit (handled by tui.c global)
 */

void screen_settings_draw (App_state *state);
void screen_settings_input(App_state *state, int ch);

#endif /* _SCREEN_SETTINGS_H_ */
