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

#ifndef PTYWRAP_H
#define PTYWRAP_H

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Session handle - opaque pointer */
typedef struct ptywrap_session ptywrap_session_t;

/* Terminal cell with attributes */
typedef struct {
    char ch;              /* UTF-8 first byte or ASCII */
    uint8_t fg_color;     /* 0-255 (ANSI 256-color) */
    uint8_t bg_color;     /* 0-255 */
    uint8_t attrs;        /* Bitfield: bold, underline, reverse, etc. */
} ptywrap_cell_t;

/* Attribute flags */
#define PTYWRAP_ATTR_BOLD      0x01
#define PTYWRAP_ATTR_UNDERLINE 0x02
#define PTYWRAP_ATTR_REVERSE   0x04
#define PTYWRAP_ATTR_BLINK     0x08

/* Return codes */
#define PTYWRAP_OK           0
#define PTYWRAP_ERR_INVAL   -1  /* Invalid argument */
#define PTYWRAP_ERR_PTY     -2  /* PTY operation failed */
#define PTYWRAP_ERR_CONTAINER -3 /* Container spawn failed */
#define PTYWRAP_ERR_THREAD  -4  /* Thread creation failed */
#define PTYWRAP_ERR_NOMEM   -5  /* Memory allocation failed */

/* Session Management */

/* Create new PTY session attached to existing container
 *
 * Args:
 *   container_id: podman container ID or name (must be running)
 *   rows: terminal height (default: 40 if 0)
 *   cols: terminal width (default: 150 if 0)
 *
 * Returns:
 *   Session handle on success, NULL on failure (errno set)
 */
ptywrap_session_t* ptywrap_create(const char *container_id,
                                   int rows, int cols);

/* Destroy session and release resources
 *
 * Note: Does NOT stop the container (per requirements)
 * Closes PTY master, joins reader thread, frees memory
 */
void ptywrap_destroy(ptywrap_session_t *session);

/* Input Operations */

/* Send characters to container shell via PTY
 *
 * Args:
 *   session: active session
 *   data: bytes to send
 *   len: number of bytes
 *
 * Returns:
 *   Number of bytes written, or negative error code
 */
int ptywrap_send(ptywrap_session_t *session, const char *data, size_t len);

/* Send null-terminated string (convenience wrapper) */
int ptywrap_send_str(ptywrap_session_t *session, const char *str);

/* Send string with newline appended */
int ptywrap_send_line(ptywrap_session_t *session, const char *str);

/* Output Operations (Synchronous) */

/* Get terminal buffer dimensions
 *
 * Args:
 *   session: active session
 *   rows: output parameter for height
 *   cols: output parameter for width
 *
 * Returns: PTYWRAP_OK or error code
 */
int ptywrap_get_size(ptywrap_session_t *session, int *rows, int *cols);

/* Get cell content at specific position
 *
 * Thread-safe: locks internal buffer mutex
 *
 * Args:
 *   session: active session
 *   row: 0-based row index
 *   col: 0-based column index
 *   cell: output parameter for cell content
 *
 * Returns: PTYWRAP_OK or PTYWRAP_ERR_INVAL if out of bounds
 */
int ptywrap_get_cell(ptywrap_session_t *session, int row, int col,
                      ptywrap_cell_t *cell);

/* Copy entire row to user buffer
 *
 * Args:
 *   session: active session
 *   row: 0-based row index
 *   cells: output buffer (must hold at least `cols` cells)
 *
 * Returns: PTYWRAP_OK or error code
 */
int ptywrap_get_row(ptywrap_session_t *session, int row,
                     ptywrap_cell_t *cells);

/* Get current cursor position
 *
 * Args:
 *   session: active session
 *   row: output parameter for cursor row
 *   col: output parameter for cursor column
 *
 * Returns: PTYWRAP_OK or error code
 */
int ptywrap_get_cursor(ptywrap_session_t *session, int *row, int *col);

/* Get text content of a row (strip attributes, just chars)
 *
 * Args:
 *   session: active session
 *   row: 0-based row index
 *   buf: output buffer
 *   buflen: buffer capacity (including null terminator)
 *
 * Returns: Number of chars written (excluding null), or error code
 */
int ptywrap_get_row_text(ptywrap_session_t *session, int row,
                          char *buf, size_t buflen);

/* Status Queries */

/* Check if container process is still running
 *
 * Returns: 1 if running, 0 if exited, negative on error
 */
int ptywrap_container_alive(ptywrap_session_t *session);

/* Get container PID (for external monitoring)
 *
 * Returns: PID or negative error code
 */
pid_t ptywrap_get_container_pid(ptywrap_session_t *session);

/* Configuration */

/* Set inter-character delay for send operations
 *
 * When set to a positive value, ptywrap_send() will insert a delay
 * between each character sent. This is useful for interactive applications
 * like vi/vim that need time to process each keystroke.
 *
 * Args:
 *   session: active session
 *   delay_ms: delay in milliseconds (0 = no delay, negative values treated as 0)
 *
 * Returns: PTYWRAP_OK or error code
 */
int ptywrap_set_send_delay(ptywrap_session_t *session, int delay_ms);

/* Get current inter-character delay setting
 *
 * Args:
 *   session: active session
 *
 * Returns: delay in milliseconds, or negative error code
 */
int ptywrap_get_send_delay(ptywrap_session_t *session);

/* Screenshot / Export Operations */

/* Take a markdown "screenshot" of the terminal buffer
 *
 * Generates a markdown-formatted representation of the terminal buffer
 * with styling annotations for colors and text attributes.
 *
 * Args:
 *   session: active session
 *   start_row: first row to capture (0-based)
 *   end_row: last row to capture (0-based, -1 for last row)
 *
 * Returns:
 *   Dynamically allocated string with markdown content (caller must free)
 *   NULL on error
 *
 * Example output:
 *   ```
 *   / # [(fg:green)ls]
 *   bin   etc   home
 *   ```
 *   *Terminal size: 150x40 | Cursor: (4,2) | Rows: 0-39*
 */
char* ptywrap_screenshot_markdown(ptywrap_session_t *session,
                                   int start_row, int end_row);

#ifdef __cplusplus
}
#endif

#endif /* PTYWRAP_H */
