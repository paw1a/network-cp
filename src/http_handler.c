#include "http_handler.h"
#include "slog.h"

#include <dirent.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

void handle_client(int client_socket) {
    char buffer[1024];
    char method[10], path[PATH_MAX], protocol[10];

    if (recv(client_socket, buffer, sizeof(buffer), 0) <= 0) {
        return;
    }

    sscanf(buffer, "%s %s %s", method, path, protocol);

    if (strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0) {
        send_error(client_socket, METHOD_NOT_ALLOWED);
        return;
    }

    char full_path[PATH_MAX];
    sprintf(full_path, "%s%s", STATIC_ROOT, path);

    char resolved_path[PATH_MAX];
    if (realpath(full_path, resolved_path) == NULL) {
        send_error(client_socket, NOT_FOUND);
        return;
    }
    if (strncmp(resolved_path, STATIC_ROOT, strlen(STATIC_ROOT)) != 0) {
        send_error(client_socket, FORBIDDEN);
        return;
    }

    if (strcmp(path, "/") == 0) {
        strcat(full_path, "index.html");
    }

    struct stat path_stat;
    if (stat(full_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
        if (path[strlen(path) - 1] != '/') {
            strcat(path, "/");
        }
        send_directory_listing(client_socket, path, full_path);
        return;
    }

    send_file(client_socket, full_path, method);
    slog_info("Response: %s", full_path);
}

void send_directory_listing(int client_socket, const char *current_path,
                            const char *directory_path) {
    DIR *directory = opendir(directory_path);
    if (directory == NULL) {
        send_error(client_socket, NOT_FOUND);
        return;
    }

    char headers[1024];
    sprintf(headers, "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n");
    send(client_socket, headers, strlen(headers), 0);

    char buffer[1024];
    sprintf(buffer,
            "<html><head><title>Directory %s</title></head><body><h1>Directory "
            "%s</h1><ul>",
            current_path, current_path);

    struct dirent *dir_entry;
    while ((dir_entry = readdir(directory)) != NULL) {
        if (strcmp(dir_entry->d_name, ".") == 0) {
            continue;
        }

        char full_entry_path[255];
        sprintf(full_entry_path, "%s/%s", directory_path, dir_entry->d_name);

        struct stat path_stat;
        if (stat(full_entry_path, &path_stat) == 0 && S_ISDIR(path_stat.st_mode)) {
            sprintf(buffer + strlen(buffer), "<li><a href=\"%s%s/\">%s/</a></li>", current_path,
                    dir_entry->d_name, dir_entry->d_name);
        } else {
            sprintf(buffer + strlen(buffer), "<li><a href=\"%s%s\">%s</a></li>", current_path,
                    dir_entry->d_name, dir_entry->d_name);
        }
    }

    strcat(buffer, "</ul></body></html>");

    send(client_socket, buffer, strlen(buffer), 0);
    slog_info("Response: list directory %s", directory_path);

    closedir(directory);
}

void send_file(int client_socket, const char *file_path, const char *method) {
    FILE *file = fopen(file_path, "rb");
    if (file == NULL) {
        send_error(client_socket, NOT_FOUND);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char headers[1024];
    sprintf(headers, "HTTP/1.1 200 OK\r\nContent-Length: %ld\r\n", file_size);

    const char *content_type = "text/plain";
    const char *file_ext = strrchr(file_path, '.');
    if (file_ext != NULL) {
        if (strcmp(file_ext, ".html") == 0) {
            content_type = "text/html";
        } else if (strcmp(file_ext, ".css") == 0) {
            content_type = "text/css";
        } else if (strcmp(file_ext, ".js") == 0) {
            content_type = "application/javascript";
        } else if (strcmp(file_ext, ".png") == 0) {
            content_type = "image/png";
        } else if (strcmp(file_ext, ".jpg") == 0 || strcmp(file_ext, ".jpeg") == 0) {
            content_type = "image/jpeg";
        } else if (strcmp(file_ext, ".swf") == 0) {
            content_type = "application/x-shockwave-flash";
        } else if (strcmp(file_ext, ".gif") == 0) {
            content_type = "image/gif";
        }
    }

    sprintf(headers + strlen(headers), "Content-Type: %s\r\n\r\n", content_type);

    send(client_socket, headers, strlen(headers), 0);

    if (strcmp(method, "HEAD") == 0) {
        fclose(file);
        return;
    }

    char buffer[1024];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

void send_error(int client_socket, int status_code) {
    char response[1024];
    sprintf(response, "HTTP/1.1 %d\r\nContent-Length: 0\r\nContent-Type: text/html\r\n\r\n",
            status_code);

    slog_info("Error %d", status_code);
    send(client_socket, response, strlen(response), 0);
}