#ifndef _SCREEN_HOME_H_
#define _SCREEN_HOME_H_

#include "tui.h"

/*
 * Home screen — first screen shown to the user.
 *
 * Layout:
 *   ┌─────────────────────────────────────────────┐
 *   │  ffpanel › Home                             │  ← title bar
 *   │                                             │
 *   │  FILE ──────────────────────────────────    │
 *   │  File:       input.mp4                      │  ← ffprobe summary
 *   │  Format:     mov,mp4  Duration: 02:34        │
 *   │  Video:      h264  1920×1080  30 fps        │
 *   │  Audio:      aac   48000 Hz  stereo         │
 *   │                                             │
 *   │  OPERATIONS ────────────────────────────    │
 *   │  ▶ Convert Format      Change container     │  ← scrollable menu
 *   │    Extract Audio       …                    │
 *   │    …                                        │
 *   ├─────────────────────────────────────────────┤
 *   │  ffpanel v0.1.0    ↑↓ Navigate  Enter …    │  ← statusbar
 *   └─────────────────────────────────────────────┘
 */

void screen_home_draw (App_state *state);
void screen_home_input(App_state *state, int ch);

#endif /* _SCREEN_HOME_H_ */
