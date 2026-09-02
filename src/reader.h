#ifndef READER_H
#define READER_H

#include "internal.h"

/* Start reader thread */
int reader_start(ptywrap_session_t *sess);

/* Stop reader thread and wait for it to finish */
void reader_stop(ptywrap_session_t *sess);

/* Reader thread function */
void* reader_thread_func(void *arg);

#endif /* READER_H */
