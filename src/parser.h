#ifndef PARSER_H
#define PARSER_H

#include "internal.h"

/* Execute CSI command (escape sequence) */
void parser_execute_csi(ptywrap_session_t *sess, char cmd);

/* Reset parser state */
void parser_reset(ptywrap_session_t *sess);

#endif /* PARSER_H */
