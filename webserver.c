#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_BUFFER 1024
#define WEB_ROOT "./www/"
#define THREAD_POOL_SIZE 5

const char *get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";

    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".mp3") == 0) return "audio/mp3";
    if (strcmp(ext, ".mp4") == 0) return "video/mp4";

    return "application/octet-stream";
}

void send_response(SOCKET client_socket, int status_code, const char *status_msg, const char *content_type, const char *body) {
    char headers[MAX_BUFFER];
    int body_length = strlen(body);

    int headers_len = snprintf(headers, sizeof(headers),
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n\r\n",
        status_code, status_msg, content_type, body_length);

    send(client_socket, headers, headers_len, 0);
    send(client_socket, body, body_length, 0);
}

void send_error_page(SOCKET client_socket, int status_code, const char *status_msg, const char *error_filename, const char *default_message) {
    char error_path[256];
    snprintf(error_path, sizeof(error_path), "%s%s", WEB_ROOT, error_filename);

    FILE *error_file = fopen(error_path, "rb");
    if (!error_file) {
        send_response(client_socket, status_code, status_msg, "text/plain", default_message);
        return;
    }

    fseek(error_file, 0, SEEK_END);
    long error_size = ftell(error_file);
    rewind(error_file);

    char *error_content = (char *)malloc(error_size + 1);
    if (!error_content) {
        fclose(error_file);
        send_response(client_socket, 500, "Internal Server Error", "text/plain", "500 - Server Error");
        return;
    }

    fread(error_content, 1, error_size, error_file);
    error_content[error_size] = '\0';

    send_response(client_socket, status_code, status_msg, "text/html", error_content);

    free(error_content);
    fclose(error_file);
}

void send_file(SOCKET client_socket, const char *filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, filename);

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        send_error_page(client_socket, 404, "Not Found", "404_error.html", "404 - File Not Found");
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char headers[MAX_BUFFER];
    int headers_len = snprintf(headers, sizeof(headers),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n\r\n",
        get_mime_type(filename), file_size);

    send(client_socket, headers, headers_len, 0);

    char buffer[MAX_BUFFER];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send(client_socket, buffer, bytes_read, 0);
    }

    fclose(file);
}

DWORD WINAPI handle_connection(LPVOID socket_ptr) {
    SOCKET client_socket = *((SOCKET *)socket_ptr);
    free(socket_ptr);

    char buffer[MAX_BUFFER] = {0};
    int bytes_received = recv(client_socket, buffer, MAX_BUFFER - 1, 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return 0;
    }

    buffer[bytes_received] = '\0';
    printf("Received Request:\n%s\n", buffer);

    char *method = strtok(buffer, " ");
    char *path = strtok(NULL, " ");
    char *protocol = strtok(NULL, "\r\n");

    if (!method || !path || !protocol) {
        send_error_page(client_socket, 400, "Bad Request", "400_error.html", "400 - Bad Request");
        closesocket(client_socket);
        return 0;
    }

    if (strcmp(method, "GET") != 0) {
        send_error_page(client_socket, 405, "Method Not Allowed", "405_error.html", "405 - Method Not Allowed");
        closesocket(client_socket);
        return 0;
    }

    const char *filename = (strcmp(path, "/") == 0) ? "index.html" : path + 1;
    send_file(client_socket, filename);

    closesocket(client_socket);
    return 0;
}

DWORD WINAPI worker_thread(LPVOID param) {
    SOCKET server_socket = *((SOCKET *)param);
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);

    while (1) {
        SOCKET client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &client_len);
        if (client_socket == INVALID_SOCKET) {
            continue;
        }

        SOCKET *client_ptr = malloc(sizeof(SOCKET));
        if (!client_ptr) {
            closesocket(client_socket);
            continue;
        }

        *client_ptr = client_socket;
        CreateThread(NULL, 0, handle_connection, client_ptr, 0, NULL);
    }

    return 0;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket;
    struct sockaddr_in server_addr;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("WSAStartup failed! Error: %d\n", WSAGetLastError());
        return 1;
    }

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Socket creation failed! Error: %d\n", WSAGetLastError());
        return 1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        printf("Bind failed! Error: %d\n", WSAGetLastError());
        return 1;
    }

    if (listen(server_socket, 5) == SOCKET_ERROR) {
        printf("Listen failed! Error: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Server started on port %d...\n", PORT);

    HANDLE threads[THREAD_POOL_SIZE];
    for (int i = 0; i < THREAD_POOL_SIZE; i++) {
        threads[i] = CreateThread(NULL, 0, worker_thread, &server_socket, 0, NULL);
    }

    WaitForMultipleObjects(THREAD_POOL_SIZE, threads, TRUE, INFINITE);
    closesocket(server_socket);
    WSACleanup();

    return 0;
}
