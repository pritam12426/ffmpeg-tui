#include "screen_home.h"
#include "widgets/statusbar.h"
#include "widgets/menu.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

/* ─── operation menu items ────────────────────────────────────────────── */

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
#define OP_ITEMS_COUNT ((int)(sizeof(OP_ITEMS) / sizeof(OP_ITEMS[0])))

static Menu g_menu        = { 0 };
static bool g_menu_inited = false;

/* ─── helpers ─────────────────────────────────────────────────────────── */

static void fmt_size(int64_t bytes, char *buf, size_t len)
{
    if (bytes <= 0)                  { snprintf(buf, len, "N/A");                            return; }
    if (bytes < 1024LL)              { snprintf(buf, len, "%lld B",    (long long)bytes);    return; }
    if (bytes < 1024LL*1024)         { snprintf(buf, len, "%.1f KB",   bytes/1024.0);        return; }
    if (bytes < 1024LL*1024*1024)    { snprintf(buf, len, "%.1f MB",   bytes/(1024.0*1024)); return; }
    snprintf(buf, len, "%.2f GB", bytes/(1024.0*1024*1024));
}

static void fmt_duration(double secs, char *buf, size_t len)
{
    if (secs < 0) { snprintf(buf, len, "N/A"); return; }
    int h = (int)(secs / 3600);
    int m = (int)((secs - h*3600) / 60);
    int s = (int)(secs - h*3600 - m*60);
    if (h > 0) snprintf(buf, len, "%d:%02d:%02d", h, m, s);
    else       snprintf(buf, len, "%02d:%02d",        m, s);
}

/* draw one "Key:  Value" row */
static int draw_kv(WINDOW *w, int row, int col,
                   const char *key, const char *val)
{
    if (row >= getmaxy(w) - 1) return row;
    wattron(w, A_DIM);
    mvwprintw(w, row, col, "%-14s", key);
    wattroff(w, A_DIM);
    wattron(w, A_BOLD | COLOR_PAIR(CP_NORMAL));
    wprintw(w, "%s", val);
    wattroff(w, A_BOLD | COLOR_PAIR(CP_NORMAL));
    return row + 1;
}

/* section header: "LABEL ─────────" */
static int draw_section(WINDOW *w, int row, int col, int width,
                         const char *label)
{
    if (row >= getmaxy(w) - 1) return row;
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(w, row, col, "%s", label);
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD);
    int llen = (int)strlen(label);
    wattron(w, A_DIM);
    mvwhline(w, row, col + llen + 1, ACS_HLINE, width - llen - col - 1);
    wattroff(w, A_DIM);
    return row + 1;
}

/* ─── draw ────────────────────────────────────────────────────────────── */

void screen_home_draw(App_state *state)
{
    WINDOW *w    = state->win_main;
    int     rows = state->term_rows - 1;   /* -1 for statusbar */
    int     cols = state->term_cols;
    int     row  = 0;

    /* ── title bar ───────────────────────────────────────────────── */
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    mvwhline(w, row, 0, ' ', cols);
    mvwprintw(w, row, 2, " ffpanel \u203a Home ");
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    row += 2;

    /* ── FILE section ────────────────────────────────────────────── */
    row = draw_section(w, row, 2, cols, "FILE");

    if (state->probe_done) {
        const Format_info *fmt = &state->probe.format;

        /* filename — truncate if too long for the terminal */
        const char *fname = state->input_file;
        char        trunc[128];
        int         max_name = cols - 18;
        if (max_name < 8) max_name = 8;
        if ((int)strlen(fname) > max_name) {
            snprintf(trunc, sizeof(trunc), "…%s",
                     fname + strlen(fname) - (size_t)(max_name - 1));
            fname = trunc;
        }

        char size_buf[16], dur_buf[16], br_buf[24];
        fmt_size(fmt->size, size_buf, sizeof(size_buf));
        fmt_duration(fmt->duration, dur_buf, sizeof(dur_buf));
        snprintf(br_buf, sizeof(br_buf), "%.0f kbps", fmt->bit_rate / 1000.0);

        row = draw_kv(w, row, 2, "File:",     fname);
        row = draw_kv(w, row, 2, "Format:",   fmt->format_name[0] ? fmt->format_name : "N/A");
        row = draw_kv(w, row, 2, "Duration:", dur_buf);
        row = draw_kv(w, row, 2, "Size:",     size_buf);
        row = draw_kv(w, row, 2, "Bitrate:",  br_buf);

        /* first video stream */
        const Stream_info *vs =
            ffprobe_first_stream(&state->probe, STREAM_TYPE_VIDEO);
        if (vs && row < rows - 6) {
            char res[48];
            snprintf(res, sizeof(res), "%s  %dx%d  %s fps  %s",
                     vs->codec_name, vs->width, vs->height,
                     vs->r_frame_rate, vs->pix_fmt);
            row = draw_kv(w, row, 2, "Video:", res);
        }

        /* first audio stream */
        const Stream_info *as =
            ffprobe_first_stream(&state->probe, STREAM_TYPE_AUDIO);
        if (as && row < rows - 5) {
            char aud[48];
            snprintf(aud, sizeof(aud), "%s  %d Hz  %s",
                     as->codec_name, as->sample_rate,
                     as->channel_layout[0] ? as->channel_layout : "?");
            row = draw_kv(w, row, 2, "Audio:", aud);
        }
    } else {
        wattron(w, COLOR_PAIR(CP_HIGHLIGHT) | A_DIM);
        mvwprintw(w, row++, 4, "Probing file…");
        wattroff(w, COLOR_PAIR(CP_HIGHLIGHT) | A_DIM);
    }

    row++;  /* gap before operations */

    /* ── OPERATIONS section ──────────────────────────────────────── */
    int menu_top  = row;
    int menu_rows = rows - menu_top - 1;   /* gap above statusbar */
    if (menu_rows < 2) menu_rows = 2;

    row = draw_section(w, row, 2, cols, "OPERATIONS");
    menu_top = row;
    menu_rows = rows - menu_top - 1;

    if (!g_menu_inited) {
        menu_init(&g_menu, OP_ITEMS, OP_ITEMS_COUNT, menu_rows);
        g_menu_inited = true;
    } else {
        g_menu.visible = menu_rows;        /* adjust on resize */
    }

    menu_draw(&g_menu, w, menu_top, 2, cols - 4);

    /* ── statusbar ───────────────────────────────────────────────── */
    statusbar_draw(state,
        "\u2191\u2193/jk Nav   Enter Select   i Info   q Quit");
}

/* ─── input ───────────────────────────────────────────────────────────── */

void screen_home_input(App_state *state, int ch)
{
    /* 'i' → full file info screen */
    if (ch == 'i' || ch == 'I') {
        state->screen = SCREEN_FILE_INFO;
        return;
    }

    /* Enter on menu item → record operation, go to operations screen */
    if (menu_input(&g_menu, ch)) {
        state->operation = (Operation)g_menu.selected;
        state->screen    = SCREEN_OPERATIONS;
    }
}
