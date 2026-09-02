/*
 * Framework for testing TTY based linux applications
 * Copyright (C) 2026 SUSE Linux 
 * 
 * Everyone is permitted to copy and distribute verbatim copies
 * of this license document, but changing it is not allowed.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 * 
 * For the full text of the GNU General Public License version 2, see:
 * https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 */

#include <string.h>
#include "parser.h"
#include "internal.h"

void parser_reset(ptywrap_session_t *sess) {
    sess->parse_state = STATE_NORMAL;
    sess->csi_param_count = 0;
    memset(sess->csi_params, 0, sizeof(sess->csi_params));
}

static void execute_sgr(ptywrap_session_t *sess) {
    /* SGR - Select Graphic Rendition */
    if (sess->csi_param_count == 0) {
        /* No params means reset */
        sess->current_attrs.fg_color = 7;
        sess->current_attrs.bg_color = 0;
        sess->current_attrs.attrs = 0;
        return;
    }

    for (int i = 0; i < sess->csi_param_count; i++) {
        int param = sess->csi_params[i];

        if (param == 0) {
            /* Reset */
            sess->current_attrs.fg_color = 7;
            sess->current_attrs.bg_color = 0;
            sess->current_attrs.attrs = 0;
        } else if (param == 1) {
            /* Bold */
            sess->current_attrs.attrs |= PTYWRAP_ATTR_BOLD;
        } else if (param == 4) {
            /* Underline */
            sess->current_attrs.attrs |= PTYWRAP_ATTR_UNDERLINE;
        } else if (param == 5) {
            /* Blink */
            sess->current_attrs.attrs |= PTYWRAP_ATTR_BLINK;
        } else if (param == 7) {
            /* Reverse */
            sess->current_attrs.attrs |= PTYWRAP_ATTR_REVERSE;
        } else if (param == 22) {
            /* Normal intensity (not bold) */
            sess->current_attrs.attrs &= ~PTYWRAP_ATTR_BOLD;
        } else if (param == 24) {
            /* Not underlined */
            sess->current_attrs.attrs &= ~PTYWRAP_ATTR_UNDERLINE;
        } else if (param == 25) {
            /* Not blinking */
            sess->current_attrs.attrs &= ~PTYWRAP_ATTR_BLINK;
        } else if (param == 27) {
            /* Not reversed */
            sess->current_attrs.attrs &= ~PTYWRAP_ATTR_REVERSE;
        } else if (param >= 30 && param <= 37) {
            /* Foreground color (8 colors) */
            sess->current_attrs.fg_color = param - 30;
        } else if (param == 38 && i + 2 < sess->csi_param_count && sess->csi_params[i + 1] == 5) {
            /* 256-color foreground: 38;5;n */
            sess->current_attrs.fg_color = sess->csi_params[i + 2];
            i += 2;
        } else if (param == 39) {
            /* Default foreground */
            sess->current_attrs.fg_color = 7;
        } else if (param >= 40 && param <= 47) {
            /* Background color (8 colors) */
            sess->current_attrs.bg_color = param - 40;
        } else if (param == 48 && i + 2 < sess->csi_param_count && sess->csi_params[i + 1] == 5) {
            /* 256-color background: 48;5;n */
            sess->current_attrs.bg_color = sess->csi_params[i + 2];
            i += 2;
        } else if (param == 49) {
            /* Default background */
            sess->current_attrs.bg_color = 0;
        }
    }
}

static void execute_cursor_move(ptywrap_session_t *sess, char cmd) {
    int n = sess->csi_param_count > 0 ? sess->csi_params[0] : 1;
    if (n == 0) n = 1;

    switch (cmd) {
    case 'A':  /* Cursor up */
        sess->cursor_row -= n;
        if (sess->cursor_row < 0) sess->cursor_row = 0;
        break;
    case 'B':  /* Cursor down */
        sess->cursor_row += n;
        if (sess->cursor_row >= sess->rows) sess->cursor_row = sess->rows - 1;
        break;
    case 'C':  /* Cursor forward */
        sess->cursor_col += n;
        if (sess->cursor_col >= sess->cols) sess->cursor_col = sess->cols - 1;
        break;
    case 'D':  /* Cursor back */
        sess->cursor_col -= n;
        if (sess->cursor_col < 0) sess->cursor_col = 0;
        break;
    }
}

static void execute_cursor_position(ptywrap_session_t *sess) {
    /* CSI row ; col H */
    int row = sess->csi_param_count > 0 ? sess->csi_params[0] : 1;
    int col = sess->csi_param_count > 1 ? sess->csi_params[1] : 1;

    /* VT100 uses 1-based indexing */
    row--;
    col--;

    if (row < 0) row = 0;
    if (row >= sess->rows) row = sess->rows - 1;
    if (col < 0) col = 0;
    if (col >= sess->cols) col = sess->cols - 1;

    sess->cursor_row = row;
    sess->cursor_col = col;
}

static void execute_erase_display(ptywrap_session_t *sess) {
    /* CSI n J */
    int n = sess->csi_param_count > 0 ? sess->csi_params[0] : 0;

    if (n == 2) {
        /* Erase entire display */
        for (int i = 0; i < sess->rows * sess->cols; i++) {
            sess->buffer[i].ch = ' ';
            sess->buffer[i].fg_color = sess->current_attrs.fg_color;
            sess->buffer[i].bg_color = sess->current_attrs.bg_color;
            sess->buffer[i].attrs = 0;
        }
        sess->cursor_row = 0;
        sess->cursor_col = 0;
    }
    /* 0 and 1 not fully implemented for brevity */
}

static void execute_erase_line(ptywrap_session_t *sess) {
    /* CSI n K */
    int n = sess->csi_param_count > 0 ? sess->csi_params[0] : 0;
    int line_start = sess->cursor_row * sess->cols;

    if (n == 0) {
        /* Erase from cursor to end of line */
        for (int i = sess->cursor_col; i < sess->cols; i++) {
            sess->buffer[line_start + i].ch = ' ';
            sess->buffer[line_start + i].fg_color = sess->current_attrs.fg_color;
            sess->buffer[line_start + i].bg_color = sess->current_attrs.bg_color;
            sess->buffer[line_start + i].attrs = 0;
        }
    } else if (n == 1) {
        /* Erase from beginning of line to cursor */
        for (int i = 0; i <= sess->cursor_col; i++) {
            sess->buffer[line_start + i].ch = ' ';
            sess->buffer[line_start + i].fg_color = sess->current_attrs.fg_color;
            sess->buffer[line_start + i].bg_color = sess->current_attrs.bg_color;
            sess->buffer[line_start + i].attrs = 0;
        }
    } else if (n == 2) {
        /* Erase entire line */
        for (int i = 0; i < sess->cols; i++) {
            sess->buffer[line_start + i].ch = ' ';
            sess->buffer[line_start + i].fg_color = sess->current_attrs.fg_color;
            sess->buffer[line_start + i].bg_color = sess->current_attrs.bg_color;
            sess->buffer[line_start + i].attrs = 0;
        }
    }
}

void parser_execute_csi(ptywrap_session_t *sess, char cmd) {
    switch (cmd) {
    case 'm':  /* SGR */
        execute_sgr(sess);
        break;
    case 'A':  /* Cursor up */
    case 'B':  /* Cursor down */
    case 'C':  /* Cursor forward */
    case 'D':  /* Cursor back */
        execute_cursor_move(sess, cmd);
        break;
    case 'H':  /* Cursor position */
    case 'f':  /* Same as H */
        execute_cursor_position(sess);
        break;
    case 'J':  /* Erase in display */
        execute_erase_display(sess);
        break;
    case 'K':  /* Erase in line */
        execute_erase_line(sess);
        break;
    default:
        /* Unsupported command - ignore */
        break;
    }
}
