#include "server.h"
#include "slog.h"

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>

void start_server(void (*handler)(int fd)) {
    int server_socket;
    struct sockaddr_in server_addr;

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("failed to bind server socket");
        exit(EXIT_FAILURE);
    }

    if (listen(server_socket, SOMAXCONN) == -1) {
        perror("failed to listen server socket");
        exit(EXIT_FAILURE);
    }

    slog_info("server started on port %d...", PORT);

    int client_sockets[MAX_CLIENTS] = {0};
    int addr_len, client_socket;
    struct sockaddr_in client_addr;
    fd_set readfds;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        int max_fd = server_socket;
        for (int j = 0; j < MAX_CLIENTS; ++j) {
            if (client_sockets[j] > 0) {
                FD_SET(client_sockets[j], &readfds);
                if (client_sockets[j] > max_fd) {
                    max_fd = client_sockets[j];
                }
            }
        }

        const int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            perror("Select error");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_socket, &readfds)) {
            addr_len = sizeof(client_addr);
            if ((client_socket = accept(server_socket, (struct sockaddr *)&client_addr,
                                        (socklen_t *)&addr_len)) == -1) {
                perror("Accept failed");
                continue;
            }

            for (int j = 0; j < MAX_CLIENTS; ++j) {
                if (client_sockets[j] == 0) {
                    client_sockets[j] = client_socket;
                    break;
                }
            }
        }

        for (int j = 0; j < MAX_CLIENTS; ++j) {
            if (client_sockets[j] > 0 && FD_ISSET(client_sockets[j], &readfds)) {
                handler(client_sockets[j]);
                client_sockets[j] = 0;
            }
        }
    }
}
