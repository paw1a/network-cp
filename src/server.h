#ifndef __SERVER_H__
#define __SERVER_H__

#define PORT 8080
#define MAX_CLIENTS 1000
#define NUM_CHILDREN 5

void start_server(void (*handler)(int fd));

#endif
