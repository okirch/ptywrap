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
#include <stdio.h>

#include "screenshot.h"
#include "internal.h"

/* ANSI color code to markdown color mapping */
static const char* ansi_to_color_name(uint8_t color) {
    /* Basic 16 colors */
    static const char* colors[] = {
        "black",    /* 0 */
        "red",      /* 1 */
        "green",    /* 2 */
        "yellow",   /* 3 */
        "blue",     /* 4 */
        "magenta",  /* 5 */
        "cyan",     /* 6 */
        "white",    /* 7 */
        "bright-black",   /* 8 */
        "bright-red",     /* 9 */
        "bright-green",   /* 10 */
        "bright-yellow",  /* 11 */
        "bright-blue",    /* 12 */
        "bright-magenta", /* 13 */
        "bright-cyan",    /* 14 */
        "bright-white"    /* 15 */
    };

    if (color < 16) {
        return colors[color];
    }
    return "default";
}

/* Escape markdown special characters */
static void append_escaped_char(char **dest, size_t *dest_size, size_t *dest_pos, char ch) {
    /* Markdown special characters that need escaping in code blocks: backticks */
    /* In fenced code blocks, most characters are literal except backticks */

    /* Ensure we have space (worst case: 2 chars for escaped char + null) */
    while (*dest_pos + 3 >= *dest_size) {
        *dest_size *= 2;
        *dest = realloc(*dest, *dest_size);
        if (!*dest) return;
    }

    /* For most characters, just append as-is */
    /* Only escape backticks in code blocks */
    if (ch == '`') {
        (*dest)[(*dest_pos)++] = '\\';
    }
    (*dest)[(*dest_pos)++] = ch;
    (*dest)[*dest_pos] = '\0';
}

/* Append string to buffer, reallocating if needed */
static void append_string(char **dest, size_t *dest_size, size_t *dest_pos, const char *str) {
    size_t len = strlen(str);

    while (*dest_pos + len + 1 >= *dest_size) {
        *dest_size *= 2;
        *dest = realloc(*dest, *dest_size);
        if (!*dest) return;
    }

    strcpy(*dest + *dest_pos, str);
    *dest_pos += len;
}

char* screenshot_to_markdown(ptywrap_session_t *sess, int start_row, int end_row) {
    if (!sess || !sess->buffer) {
        return NULL;
    }

    /* Validate row ranges */
    if (start_row < 0) start_row = 0;
    if (end_row < 0 || end_row >= sess->rows) end_row = sess->rows - 1;
    if (start_row > end_row) {
        int tmp = start_row;
        start_row = end_row;
        end_row = tmp;
    }

    /* Allocate initial buffer (will grow as needed) */
    size_t buffer_size = 4096;
    size_t buffer_pos = 0;
    char *output = malloc(buffer_size);
    if (!output) {
        return NULL;
    }
    output[0] = '\0';

    /* Lock buffer for reading */
    pthread_mutex_lock(&sess->buffer_lock);

    /* Start markdown code block */
    append_string(&output, &buffer_size, &buffer_pos, "```\n");

    /* Process each row */
    for (int row = start_row; row <= end_row; row++) {
        int line_start = row * sess->cols;

        /* Track current formatting to minimize markdown annotations */
        uint8_t current_fg = 7;  /* default white */
        uint8_t current_bg = 0;  /* default black */
        uint8_t current_attrs = 0;
        int in_styled_section = 0;

        /* Process each cell in the row */
        for (int col = 0; col < sess->cols; col++) {
            ptywrap_cell_t cell = sess->buffer[line_start + col];

            /* Check if formatting changed */
            int formatting_changed = (cell.fg_color != current_fg ||
                                     cell.bg_color != current_bg ||
                                     cell.attrs != current_attrs);

            if (formatting_changed) {
                /* Close previous styled section if any */
                if (in_styled_section) {
                    append_string(&output, &buffer_size, &buffer_pos, "]");
                    in_styled_section = 0;
                }

                /* Check if we need styling (non-default) */
                int needs_styling = (cell.fg_color != 7 ||
                                    cell.bg_color != 0 ||
                                    cell.attrs != 0);

                if (needs_styling) {
                    /* Start new styled section with annotation */
                    append_string(&output, &buffer_size, &buffer_pos, "[");

                    /* Add styling hints in parentheses before the text */
                    char style_hint[128];

                    if (cell.attrs & PTYWRAP_ATTR_BOLD) {
                        snprintf(style_hint, sizeof(style_hint), "(bold)");
                        append_string(&output, &buffer_size, &buffer_pos, style_hint);
                    }
                    if (cell.attrs & PTYWRAP_ATTR_UNDERLINE) {
                        snprintf(style_hint, sizeof(style_hint), "(underline)");
                        append_string(&output, &buffer_size, &buffer_pos, style_hint);
                    }
                    if (cell.attrs & PTYWRAP_ATTR_REVERSE) {
                        snprintf(style_hint, sizeof(style_hint), "(reverse)");
                        append_string(&output, &buffer_size, &buffer_pos, style_hint);
                    }
                    if (cell.fg_color != 7) {
                        snprintf(style_hint, sizeof(style_hint), "(fg:%s)",
                                ansi_to_color_name(cell.fg_color));
                        append_string(&output, &buffer_size, &buffer_pos, style_hint);
                    }
                    if (cell.bg_color != 0) {
                        snprintf(style_hint, sizeof(style_hint), "(bg:%s)",
                                ansi_to_color_name(cell.bg_color));
                        append_string(&output, &buffer_size, &buffer_pos, style_hint);
                    }

                    in_styled_section = 1;
                }

                current_fg = cell.fg_color;
                current_bg = cell.bg_color;
                current_attrs = cell.attrs;
            }

            /* Append the character */
            append_escaped_char(&output, &buffer_size, &buffer_pos, cell.ch);
        }

        /* Close any open styled section at end of line */
        if (in_styled_section) {
            append_string(&output, &buffer_size, &buffer_pos, "]");
        }

        /* Add newline */
        append_string(&output, &buffer_size, &buffer_pos, "\n");
    }

    /* End markdown code block */
    append_string(&output, &buffer_size, &buffer_pos, "```\n");

    /* Add metadata footer */
    char footer[256];
    snprintf(footer, sizeof(footer),
             "\n*Terminal size: %dx%d | Cursor: (%d,%d) | Rows: %d-%d*\n",
             sess->cols, sess->rows,
             sess->cursor_col, sess->cursor_row,
             start_row, end_row);
    append_string(&output, &buffer_size, &buffer_pos, footer);

    pthread_mutex_unlock(&sess->buffer_lock);

    return output;
}
