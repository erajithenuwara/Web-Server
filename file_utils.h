#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <windows.h>
#include <sys/stat.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define MAX_BUFFER 1024
#define WEB_ROOT "./www/"
#define THREAD_POOL_SIZE 5

const char *get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream"; // Default binary type

    if (strcmp(ext, ".html") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    return "application/octet-stream";
}
void send_file(SOCKET client_socket, const char *filename) {
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s%s", WEB_ROOT, filename);

    FILE *file = fopen(filepath, "rb");
    if (!file) {
        const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 - File Not Found";
        send(client_socket, not_found, strlen(not_found), 0);
        return;
    }

    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);

    char headers[MAX_BUFFER];
    int headers_len = snprintf(headers, sizeof(headers), 
        "HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %ld\r\n\r\n", 
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
    int bytes_received = recv(client_socket, buffer, MAX_BUFFER, 0);
    if (bytes_received <= 0) {
        closesocket(client_socket);
        return 0;
    }

    char method[10], path[256];
    if (sscanf(buffer, "%s %s", method, path) != 2) {
        const char *bad_request = "HTTP/1.1 400 Bad Request\r\nContent-Type: text/plain\r\n\r\n400 - Bad Request";
        send(client_socket, bad_request, strlen(bad_request), 0);
        closesocket(client_socket);
        return 0;
    }

    const char *filename = (strcmp(path, "/") == 0) ? "index.html" : path + 1;
    send_file(client_socket, filename);

    closesocket(client_socket);
    return 0;
}

DWORD WINAPI worker_thread(LPVOID param) {
    SOCKET *server_socket = (SOCKET *)param;
    struct sockaddr_in client_addr;
    int client_len = sizeof(client_addr);

    while (1) {
        SOCKET *client_socket = malloc(sizeof(SOCKET));
        if ((*client_socket = accept(*server_socket, (struct sockaddr *)&client_addr, &client_len)) != INVALID_SOCKET) {
            CreateThread(NULL, 0, handle_connection, client_socket, 0, NULL);
        } else {
            free(client_socket);
        }
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

