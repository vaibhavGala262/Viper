#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#include "log.h"
#include "client.h"
#include "router.h"

#define PORT 8080

int main()
{
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        write_log("ERROR", "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        write_log("ERROR", "setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Set up address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    init_routes();

    // Bind socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        write_log("ERROR", "Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_fd, 10) < 0)
    {
        perror("Listen failed");
        write_log("ERROR", "Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);

    while (1)
    {
        int *client_sock = malloc(sizeof(int));
        if (!client_sock)
        {
            write_log("ERROR", "Memory allocation failed");
            continue;
        }

        *client_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (*client_sock < 0)
        {
            write_log("ERROR", "Accept failed");
            free(client_sock);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock) != 0)
        {
            write_log("ERROR", "Thread creation failed");
            close(*client_sock);
            free(client_sock);
        }

        pthread_detach(tid); // Automatically reclaim resources when thread exits
    }

    close(server_fd);
    return 0;
}
