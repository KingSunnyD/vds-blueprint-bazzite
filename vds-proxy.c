#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>

#define UNIX_PATH "/tmp/vds-proxy-socket"

void handle_client(int client_fd) {
    char buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(client_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Received: %s", buffer);

        // Process the received data and send a response
        const char *response = "Data received\n";
        write(client_fd, response, strlen(response));
    }

    if (bytes_read < 0) {
        perror("read");
    }

    close(client_fd);
}

int main() {
    int server_fd, client_fd;
    struct sockaddr_un server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // Create a UNIX domain socket
    if ((server_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Remove any existing socket file
    unlink(UNIX_PATH);

    // Bind the socket to the specified path
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, UNIX_PATH, sizeof(server_addr.sun_path) - 1);

    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("vDS Proxy is running. Waiting for clients...\n");

    while (1) {
        // Accept a client connection
        if ((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len)) == -1) {
            perror("accept");
            continue;
        }

        printf("New client connected\n");

        // Handle the client connection in a separate process
        pid_t pid = fork();
        if (pid == 0) {
            // Child process
            close(server_fd);
            handle_client(client_fd);
            exit(EXIT_SUCCESS);
        } else if (pid > 0) {
            // Parent process
            close(client_fd);
        } else {
            // Error handling for fork
            perror("fork");
            close(client_fd);
        }
    }

    close(server_fd);
    return 0;
}
