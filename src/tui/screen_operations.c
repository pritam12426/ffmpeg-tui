#include "screen_operations.h"
#include "widgets/statusbar.h"
#include "widgets/menu.h"

#include <ncurses.h>
#include <string.h>

/* ─── data ────────────────────────────────────────────────────────────── */

static const Menu_item OP_ITEMS[] = {
    { "Convert Format",    "Change container or codec"        },
    { "Extract Audio",     "Strip video, keep audio"          },
    { "Resize / Scale",    "Change output resolution"         },
    { "Trim / Cut",        "Cut segment by time"              },
    { "Compress",          "Reduce file size via CRF"         },
    { "Merge / Concat",    "Join multiple files"              },
    { "Add Subtitles",     "Burn-in or embed subtitles"       },
    { "Create GIF",        "Animated GIF from video"          },
    { "Extract Thumbnails","Grab frames as PNG images"        },
    { "Watermark",         "Overlay image or text"            },
    { "Audio Adjustments", "Normalize, volume, fade"          },
    { "Video Filters",     "Stabilize, speed, rotate, flip"  },
};
#define OP_COUNT_LOCAL ((int)(sizeof(OP_ITEMS) / sizeof(OP_ITEMS[0])))

/* one paragraph per operation, matching OP_ITEMS order */
static const char *OP_DESC[OP_COUNT_LOCAL] = {
    /* Convert Format */
    "Re-encode or remux into a different container and codec.\n"
    "Choose output format (mp4, mkv, webm…), video codec\n"
    "(H.264, H.265, AV1, VP9), audio codec, CRF quality,\n"
    "and encoding preset.",

    /* Extract Audio */
    "Remove the video track and export only the audio stream.\n"
    "Supports mp3, aac, flac, wav, ogg, and opus output.\n"
    "Set the target bitrate for the output audio file.",

    /* Resize / Scale */
    "Scale the video to a standard resolution while keeping\n"
    "the original aspect ratio.\n"
    "Choose from 4K, 1080p, 720p, 480p, or 360p.\n"
    "Output codec defaults to H.264.",

    /* Trim / Cut */
    "Cut a portion of the video by specifying start and end\n"
    "time (HH:MM:SS).\n"
    "Lossless stream-copy avoids re-encoding and is instant;\n"
    "re-encode mode gives frame-accurate cuts.",

    /* Compress */
    "Reduce the output file size by tuning CRF quality and\n"
    "encoding preset.\n"
    "Higher CRF value = smaller file, lower quality.\n"
    "Audio is stream-copied without re-encoding.",

    /* Merge / Concat */
    "Join multiple media files of the same format.\n"
    "Files in the same directory with the same extension are\n"
    "concatenated in alphabetical order.\n"
    "Stream-copy keeps the operation lossless and fast.",

    /* Add Subtitles */
    "Burn a subtitle file into the video (hard-sub) or\n"
    "embed it as a selectable stream (soft-sub).\n"
    "Supports SRT, ASS, and VTT subtitle formats.",

    /* Create GIF */
    "Export a looping animated GIF from a video segment.\n"
    "Set start time, duration, frame rate, and output width.\n"
    "Uses ffmpeg palette optimisation for smaller, sharper GIFs.",

    /* Extract Thumbnails */
    "Capture a single frame from the video as a PNG image.\n"
    "Specify the exact timestamp to grab.\n"
    "Future: contact sheet (4x4 grid) and interval mode.",

    /* Watermark */
    "Overlay a watermark at one of five screen positions:\n"
    "top-left, top-right, center, bottom-left, bottom-right.\n"
    "Control opacity and scale of the watermark image.",

    /* Audio Adjustments */
    "Adjust the audio track without touching the video.\n"
    "Options: re-encode to a different codec, set target\n"
    "bitrate, normalize loudness, or strip the audio entirely.",

    /* Video Filters */
    "Apply common video filters:\n"
    "  \xe2\x80\xa2 Speed change  (0.5x slow-mo or 2x fast)\n"
    "  \xe2\x80\xa2 Horizontal / vertical flip\n"
    "  \xe2\x80\xa2 Rotation (90 \xc2\xb0, 180 \xc2\xb0, 270 \xc2\xb0)",
};

static Menu g_menu        = { 0 };
static bool g_menu_inited = false;

/* ─── draw ────────────────────────────────────────────────────────────── */

void screen_operations_draw(App_state *state)
{
    WINDOW *w    = state->win_main;
    int     rows = state->term_rows - 1;
    int     cols = state->term_cols;
    int     row  = 0;

    /* ── title bar ───────────────────────────────────────────────── */
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    mvwhline(w, row, 0, ' ', cols);
    mvwprintw(w, row, 2, " ffpanel \u203a Choose Operation ");
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    row++;

    /* ── split-pane layout ───────────────────────────────────────── */
    int menu_width = (cols * 55) / 100;     /* left 55%  */
    int div_col    = menu_width;
    int desc_col   = div_col + 2;
    int desc_width = cols - desc_col - 1;
    int menu_top   = row + 1;               /* +1 for sub-header     */
    int menu_rows  = rows - menu_top - 1;
    if (menu_rows < 2) menu_rows = 2;

    /* vertical divider */
    wattron(w, A_DIM);
    for (int r = 1; r < rows; r++)
        mvwaddch(w, r, div_col, ACS_VLINE);
    wattroff(w, A_DIM);

    /* column headers */
    wattron(w, A_DIM);
    mvwprintw(w, row, 2,        "Operations (%d)", OP_COUNT_LOCAL);
    mvwprintw(w, row, desc_col, "Description");
    wattroff(w, A_DIM);

    /* init menu once */
    if (!g_menu_inited) {
        menu_init(&g_menu, OP_ITEMS, OP_COUNT_LOCAL, menu_rows);
        g_menu_inited = true;
    } else {
        g_menu.visible = menu_rows;    /* update on resize */
    }

    menu_draw(&g_menu, w, menu_top, 1, menu_width - 1);

    /* ── description panel ───────────────────────────────────────── */
    if (desc_width > 12) {
        int sel = g_menu.selected;

        /* op name in bold */
        wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD);
        mvwprintw(w, menu_top, desc_col, "%s", OP_ITEMS[sel].label);
        wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD);

        /* word-wrap description */
        const char *desc = OP_DESC[sel];
        int         drow = menu_top + 2;
        char        line[256];
        int         lpos = 0;

        for (int i = 0; desc[i] && drow < rows - 1; i++) {
            if (desc[i] == '\n' || lpos >= desc_width - 1) {
                line[lpos] = '\0';
                mvwprintw(w, drow++, desc_col, "%s", line);
                lpos = 0;
                if (desc[i] == '\n') continue;
            }
            line[lpos++] = desc[i];
        }
        if (lpos > 0 && drow < rows - 1) {
            line[lpos] = '\0';
            mvwprintw(w, drow, desc_col, "%s", line);
        }

        /* hint at the bottom of description panel */
        wattron(w, A_DIM);
        mvwprintw(w, rows - 2, desc_col, "i \u2192 file info");
        wattroff(w, A_DIM);
    }

    statusbar_draw(state,
        "\u2191\u2193/jk Nav   Enter Select   i Info   Esc Back   q Quit");
}

/* ─── input ───────────────────────────────────────────────────────────── */

void screen_operations_input(App_state *state, int ch)
{
    switch (ch) {
    case 27:    /* Esc */
    case 'h':
    case 'H':
        state->screen = SCREEN_HOME;
        return;

    case 'i':
    case 'I':
        state->screen = SCREEN_FILE_INFO;
        return;

    default:
        break;
    }

    if (menu_input(&g_menu, ch)) {
        state->operation = (Operation)g_menu.selected;
        state->screen    = SCREEN_SETTINGS;
    }
}
