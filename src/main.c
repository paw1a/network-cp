#include <stdlib.h>
#include <unistd.h>

#include "tpool.h"
#include "server.h"
#include "http_handler.h"
#include "slog.h"

#define TPOOL_SIZE 10

static tpool_t *pool;

void worker(void *arg) {
    const int *val = arg;
    const int fd = *val;
    handle_client(fd);
    close(fd);
}

void handler(int fd) {
    int *fd_ptr = malloc(sizeof(int));
    *fd_ptr = fd;
    tpool_add_work(pool, worker, fd_ptr);
}

int main(int argc, char **argv) {
    int nEnabledLevels = SLOG_NOTAG | SLOG_ERROR;
    nEnabledLevels |= SLOG_WARN | SLOG_FATAL | SLOG_INFO;
    slog_init("logfile", nEnabledLevels, 0);
    pool = tpool_create(TPOOL_SIZE);
    start_server(handler);
    tpool_wait(pool);
    tpool_destroy(pool);
    return 0;
}
