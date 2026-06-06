#include "screen_file_info.h"
#include "widgets/statusbar.h"

#include <ncurses.h>
#include <stdio.h>
#include <string.h>

/* ─── helpers ─────────────────────────────────────────────────────────── */

static int section_hdr(WINDOW *w, int row, int col, int width,
                        const char *label)
{
    if (row >= getmaxy(w) - 1) return row;
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD);
    mvwprintw(w, row, col, "%s", label);
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD);
    wattron(w, A_DIM);
    mvwhline(w, row, col + (int)strlen(label) + 1,
             ACS_HLINE, width - (int)strlen(label) - col - 1);
    wattroff(w, A_DIM);
    return row + 1;
}

static int kv(WINDOW *w, int row, int col,
               const char *key, const char *val)
{
    if (row >= getmaxy(w) - 1) return row;
    wattron(w, A_DIM);
    mvwprintw(w, row, col, "  %-20s", key);
    wattroff(w, A_DIM);
    wattron(w, A_BOLD);
    wprintw(w, "%s", val);
    wattroff(w, A_BOLD);
    return row + 1;
}

static void fmt_size(int64_t bytes, char *buf, size_t len)
{
    if (bytes <= 0)               { snprintf(buf, len, "N/A");                            return; }
    if (bytes < 1024LL)           { snprintf(buf, len, "%lld B",    (long long)bytes);    return; }
    if (bytes < 1024LL*1024)      { snprintf(buf, len, "%.1f KB",   bytes/1024.0);        return; }
    if (bytes < 1024LL*1024*1024) { snprintf(buf, len, "%.1f MB",   bytes/(1024.0*1024)); return; }
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

static const char *stream_type_name(Stream_type t)
{
    switch (t) {
    case STREAM_TYPE_VIDEO:    return "Video";
    case STREAM_TYPE_AUDIO:    return "Audio";
    case STREAM_TYPE_SUBTITLE: return "Subtitle";
    case STREAM_TYPE_DATA:     return "Data";
    default:                   return "Unknown";
    }
}

/* ─── draw ────────────────────────────────────────────────────────────── */

void screen_file_info_draw(App_state *state)
{
    WINDOW *w    = state->win_main;
    int     cols = state->term_cols;
    int     rows = state->term_rows - 1;
    int     row  = 0;

    /* ── title bar ───────────────────────────────────────────────── */
    wattron(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    mvwhline(w, row, 0, ' ', cols);
    mvwprintw(w, row, 2, " ffpanel \u203a File Info ");
    wattroff(w, COLOR_PAIR(CP_TITLE) | A_BOLD | A_REVERSE);
    row += 2;

    if (!state->probe_done) {
        wattron(w, COLOR_PAIR(CP_HIGHLIGHT));
        mvwprintw(w, row, 2, "No probe data available for: %s",
                  state->input_file ? state->input_file : "(none)");
        wattroff(w, COLOR_PAIR(CP_HIGHLIGHT));
        statusbar_draw(state, "Esc/h Back   q Quit");
        return;
    }

    const Format_info *fmt = &state->probe.format;

    /* ── FORMAT section ──────────────────────────────────────────── */
    row = section_hdr(w, row, 2, cols, "FORMAT");

    char size_buf[16], dur_buf[16], br_buf[24];
    fmt_size(fmt->size,         size_buf, sizeof(size_buf));
    fmt_duration(fmt->duration, dur_buf,  sizeof(dur_buf));
    snprintf(br_buf, sizeof(br_buf), "%.0f kbps", fmt->bit_rate / 1000.0);

    row = kv(w, row, 2, "Filename:",     fmt->filename[0]         ? fmt->filename       : state->input_file);
    row = kv(w, row, 2, "Format:",       fmt->format_long_name[0] ? fmt->format_long_name : fmt->format_name);
    row = kv(w, row, 2, "Duration:",     dur_buf);
    row = kv(w, row, 2, "File size:",    size_buf);
    row = kv(w, row, 2, "Bit rate:",     br_buf);
    if (fmt->title[0])   row = kv(w, row, 2, "Title:",   fmt->title);
    if (fmt->artist[0])  row = kv(w, row, 2, "Artist:",  fmt->artist);
    if (fmt->encoder[0]) row = kv(w, row, 2, "Encoder:", fmt->encoder);
    row++;

    /* ── STREAMS section ─────────────────────────────────────────── */
    if (row < rows - 2)
        row = section_hdr(w, row, 2, cols, "STREAMS");

    for (int i = 0; i < state->probe.stream_count && row < rows - 2; i++) {
        const Stream_info *st = &state->probe.streams[i];

        /* stream label line */
        wattron(w, COLOR_PAIR(CP_OK) | A_BOLD);
        mvwprintw(w, row++, 4, "[%d] %s  —  %s",
                  st->index,
                  stream_type_name(st->type),
                  st->codec_long_name[0] ? st->codec_long_name : st->codec_name);
        wattroff(w, COLOR_PAIR(CP_OK) | A_BOLD);

        if (st->type == STREAM_TYPE_VIDEO && row < rows - 2) {
            char res[64], fps[24], vbr[24];
            snprintf(res, sizeof(res), "%dx%d  %s", st->width, st->height, st->pix_fmt);
            snprintf(fps, sizeof(fps), "%s fps", st->r_frame_rate);
            snprintf(vbr, sizeof(vbr), "%.0f kbps", st->bit_rate / 1000.0);
            row = kv(w, row, 4, "Resolution:", res);
            row = kv(w, row, 4, "Frame rate:", fps);
            if (st->bit_rate > 0) row = kv(w, row, 4, "Bit rate:", vbr);
        } else if (st->type == STREAM_TYPE_AUDIO && row < rows - 2) {
            char sr[24], ch[32], abr[24], dur[16];
            snprintf(sr,  sizeof(sr),  "%d Hz", st->sample_rate);
            snprintf(ch,  sizeof(ch),  "%s  (%d ch)", st->channel_layout, st->channels);
            snprintf(abr, sizeof(abr), "%.0f kbps", st->bit_rate / 1000.0);
            fmt_duration(st->duration, dur, sizeof(dur));
            row = kv(w, row, 4, "Sample rate:", sr);
            row = kv(w, row, 4, "Channels:",    ch);
            if (st->bit_rate > 0) row = kv(w, row, 4, "Bit rate:", abr);
            if (st->duration > 0) row = kv(w, row, 4, "Duration:", dur);
        }

        if (st->language[0]) row = kv(w, row, 4, "Language:", st->language);
        row++;   /* gap between streams */
    }

    statusbar_draw(state, "Esc/h Back   q Quit");
}

/* ─── input ───────────────────────────────────────────────────────────── */

void screen_file_info_input(App_state *state, int ch)
{
    switch (ch) {
    case 27:    /* Esc */
    case 'h':
    case 'H':
        state->screen = SCREEN_HOME;
        break;
    default:
        break;
    }
}
