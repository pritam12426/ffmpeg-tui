#ifndef _SCREEN_FILE_INFO_H_
#define _SCREEN_FILE_INFO_H_

#include "tui.h"

/*
 * File Info screen — full ffprobe metadata dump.
 *
 * Reached by pressing 'i' from Home or Operations.
 * Shows: format info, all streams with detailed fields.
 * Press Esc / h to go back.
 */

void screen_file_info_draw (App_state *state);
void screen_file_info_input(App_state *state, int ch);

#endif /* _SCREEN_FILE_INFO_H_ */
