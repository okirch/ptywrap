#ifndef INTERNAL_H
#define INTERNAL_H

#include <pthread.h>
#include <sys/types.h>
#include "../include/ptywrap.h"

/* Parser states for VT100/ANSI escape sequences */
enum parse_state {
    STATE_NORMAL,
    STATE_ESC,
    STATE_CSI,
    STATE_CSI_PARAM
};

/* Session structure (opaque to users) */
struct ptywrap_session {
    /* PTY state */
    int master_fd;         /* PTY master file descriptor */
    char *slave_name;      /* PTY slave device path */

    /* Container state */
    pid_t exec_pid;        /* Podman exec process PID */
    char *container_id;    /* Container ID/name */

    /* Terminal buffer */
    int rows;
    int cols;
    ptywrap_cell_t *buffer; /* Flat array: row-major order */
    int cursor_row;
    int cursor_col;
    ptywrap_cell_t current_attrs; /* Current SGR attributes */

    /* Synchronization */
    pthread_mutex_t buffer_lock;
    pthread_t reader_thread;
    volatile int reader_running;

    /* Parser state (for VT100/ANSI) */
    enum parse_state parse_state;
    int csi_params[16];
    int csi_param_count;

    /* Inter-character delay (milliseconds) */
    int send_delay_ms;
};

#endif /* INTERNAL_H */
