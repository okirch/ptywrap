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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "terminal.h"
#include "parser.h"
#include "internal.h"

int terminal_init(ptywrap_session_t *sess) {
    if (!sess) {
        return PTYWRAP_ERR_INVAL;
    }

    /* Allocate buffer */
    size_t buffer_size = sess->rows * sess->cols;
    sess->buffer = calloc(buffer_size, sizeof(ptywrap_cell_t));
    if (!sess->buffer) {
        return PTYWRAP_ERR_NOMEM;
    }

    /* Initialize all cells with defaults */
    for (size_t i = 0; i < buffer_size; i++) {
        sess->buffer[i].ch = ' ';
        sess->buffer[i].fg_color = 7;  /* Default white */
        sess->buffer[i].bg_color = 0;  /* Default black */
        sess->buffer[i].attrs = 0;
    }

    /* Initialize cursor and attributes */
    sess->cursor_row = 0;
    sess->cursor_col = 0;
    sess->current_attrs.ch = 0;
    sess->current_attrs.fg_color = 7;
    sess->current_attrs.bg_color = 0;
    sess->current_attrs.attrs = 0;

    /* Initialize parser state */
    parser_reset(sess);

    return PTYWRAP_OK;
}

void terminal_free(ptywrap_session_t *sess) {
    if (sess && sess->buffer) {
        free(sess->buffer);
        sess->buffer = NULL;
    }
}

void terminal_put_char(ptywrap_session_t *sess, char ch) {
    if (sess->cursor_row >= sess->rows || sess->cursor_col >= sess->cols) {
        return;
    }

    /* Calculate buffer index */
    int idx = sess->cursor_row * sess->cols + sess->cursor_col;

    /* Write character with current attributes */
    sess->buffer[idx].ch = ch;
    sess->buffer[idx].fg_color = sess->current_attrs.fg_color;
    sess->buffer[idx].bg_color = sess->current_attrs.bg_color;
    sess->buffer[idx].attrs = sess->current_attrs.attrs;

    /* Advance cursor */
    sess->cursor_col++;
    if (sess->cursor_col >= sess->cols) {
        terminal_newline(sess);
    }
}

void terminal_newline(ptywrap_session_t *sess) {
    sess->cursor_col = 0;
    sess->cursor_row++;

    /* Scroll if needed */
    if (sess->cursor_row >= sess->rows) {
        /* Move all lines up by one */
        size_t line_size = sess->cols * sizeof(ptywrap_cell_t);
        memmove(sess->buffer, sess->buffer + sess->cols,
                (sess->rows - 1) * line_size);

        /* Clear last line */
        int last_line_start = (sess->rows - 1) * sess->cols;
        for (int i = 0; i < sess->cols; i++) {
            sess->buffer[last_line_start + i].ch = ' ';
            sess->buffer[last_line_start + i].fg_color = sess->current_attrs.fg_color;
            sess->buffer[last_line_start + i].bg_color = sess->current_attrs.bg_color;
            sess->buffer[last_line_start + i].attrs = 0;
        }

        sess->cursor_row = sess->rows - 1;
    }
}

void terminal_process_bytes(ptywrap_session_t *sess, const char *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        char ch = data[i];

        switch (sess->parse_state) {
        case STATE_NORMAL:
            if (ch == '\033') {
                sess->parse_state = STATE_ESC;
            } else if (ch == '\n') {
                terminal_newline(sess);
            } else if (ch == '\r') {
                sess->cursor_col = 0;
            } else if (ch == '\b') {
                /* Backspace */
                if (sess->cursor_col > 0) {
                    sess->cursor_col--;
                }
            } else if (ch >= 32 && ch < 127) {
                /* Printable ASCII */
                terminal_put_char(sess, ch);
            }
            /* Ignore other control characters */
            break;

        case STATE_ESC:
            if (ch == '[') {
                sess->parse_state = STATE_CSI;
                sess->csi_param_count = 0;
                memset(sess->csi_params, 0, sizeof(sess->csi_params));
            } else {
                /* Unknown escape - ignore and return to normal */
                sess->parse_state = STATE_NORMAL;
            }
            break;

        case STATE_CSI:
        case STATE_CSI_PARAM:
            if (isdigit(ch)) {
                /* Accumulate parameter */
                sess->parse_state = STATE_CSI_PARAM;
                sess->csi_params[sess->csi_param_count] =
                    sess->csi_params[sess->csi_param_count] * 10 + (ch - '0');
            } else if (ch == ';') {
                sess->csi_param_count++;
                if (sess->csi_param_count >= 16) {
                    sess->csi_param_count = 15;
                }
                sess->parse_state = STATE_CSI_PARAM;
            } else {
                /* Terminator - execute CSI command */
                if (sess->parse_state == STATE_CSI_PARAM) {
                    sess->csi_param_count++;
                }
                parser_execute_csi(sess, ch);
                sess->parse_state = STATE_NORMAL;
            }
            break;
        }
    }
}
