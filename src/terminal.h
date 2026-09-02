#ifndef TERMINAL_H
#define TERMINAL_H

#include "internal.h"

/* Initialize terminal buffer */
int terminal_init(ptywrap_session_t *sess);

/* Free terminal buffer */
void terminal_free(ptywrap_session_t *sess);

/* Process incoming bytes from PTY (VT100/ANSI parsing) */
void terminal_process_bytes(ptywrap_session_t *sess, const char *data, size_t len);

/* Put a character at current cursor position and advance */
void terminal_put_char(ptywrap_session_t *sess, char ch);

/* Move cursor to new line */
void terminal_newline(ptywrap_session_t *sess);

#endif /* TERMINAL_H */
