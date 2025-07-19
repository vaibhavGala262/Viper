// test logging thread id
#include "client.h"
#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *handle_client(void *arg) {

    


    pthread_t tid = pthread_self();
    char log[256];
    snprintf(log, sizeof(log), "Thread %lu handling client", (unsigned long)tid);
    write_log("INFO", log);
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};

    // Log client IP address
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Connection from IP: %s", client_ip);
    write_log("INFO", log_msg);

    // Read request
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0) {
        buffer[bytes_read] = '\0';  // Null-terminate
        write_log("INFO", buffer);  // Log raw request

        // Send response
        const char *response =
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Hello, World!";

        write(client_socket, response, strlen(response));
        write_log("INFO", "Sent 'Hello, World!' response.");
    } else {
        write_log("ERROR", "Failed to read from client");
    }

    close(client_socket);
    return NULL;
}
