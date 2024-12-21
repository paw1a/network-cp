#ifndef __HTTP_HANDLER_H__
#define __HTTP_HANDLER_H__

#define FORBIDDEN 403
#define NOT_FOUND 404
#define METHOD_NOT_ALLOWED 405

#define STATIC_ROOT "/Users/paw1a/study/network-cp/static"

void send_directory_listing(int client_socket, const char *current_path,
                            const char *directory_path);
void handle_client(int client_socket);
void send_file(int client_socket, const char *file_path, const char *method);
void send_error(int client_socket, int status_code);

#endif
