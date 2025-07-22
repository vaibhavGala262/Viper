#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

// Add this for struct sigaction on some systems
#ifdef __linux__
#define _POSIX_C_SOURCE 200809L
#endif

#include "log.h"
#include "client.h"
#include "router.h"
#include "globals.h"  // Our new globals header

#define PORT 8080

int main()
{
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Initialize global variables
    init_server_globals();

    // Register signal handlers - using simpler signal() function instead of sigaction
    if (signal(SIGINT, signal_handler) == SIG_ERR) {
        perror("Cannot handle SIGINT");
        write_log("ERROR", "Cannot register SIGINT handler");
        exit(EXIT_FAILURE);
    }
    
    if (signal(SIGTERM, signal_handler) == SIG_ERR) {
        perror("Cannot handle SIGTERM");
        write_log("ERROR", "Cannot register SIGTERM handler");
        exit(EXIT_FAILURE);
    }
    
    // Ignore SIGPIPE to prevent crashes from broken connections
    signal(SIGPIPE, SIG_IGN);

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        write_log("ERROR", "Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt failed");
        write_log("ERROR", "setsockopt failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Set up address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    init_routes();

    // Bind socket
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        write_log("ERROR", "Bind failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Start listening
    if (listen(server_socket, 10) < 0)
    {
        perror("Listen failed");
        write_log("ERROR", "Listen failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);
    printf("Press Ctrl+C for graceful shutdown\n");
    write_log("INFO", "Server started successfully");

    // Main server loop with shutdown checking
    while (!server_shutdown)
    {
        int *client_sock = malloc(sizeof(int));
        if (!client_sock)
        {
            write_log("ERROR", "Memory allocation failed");
            continue;
        }

        *client_sock = accept(server_socket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        
        if (*client_sock < 0)
        {
            free(client_sock);
            
            if (server_shutdown) {
                printf("[INFO] Server socket closed. Stopping accept loop.\n");
                write_log("INFO", "Accept loop stopped due to shutdown");
                break;
            }
            
            if (errno == EINTR) {
                // Interrupted by signal, check shutdown flag and continue
                continue;
            }
            
            write_log("ERROR", "Accept failed");
            continue;
        }

        // Check shutdown flag before creating new thread
        if (server_shutdown) {
            printf("[INFO] Shutdown in progress. Rejecting new connection.\n");
            write_log("INFO", "Rejecting new connection due to shutdown");
            close(*client_sock);
            free(client_sock);
            break;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, handle_client, client_sock) != 0)
        {
            write_log("ERROR", "Thread creation failed");
            close(*client_sock);
            free(client_sock);
        }
        else
        {
            pthread_detach(tid); // Automatically reclaim resources when thread exits
        }
    }

    // Graceful shutdown sequence
    printf("[INFO] Graceful shutdown initiated...\n");
    write_log("INFO", "Graceful shutdown initiated");
    
    // Wait for active connections to finish (with connection tracking)
    printf("[INFO] Waiting for active connections to complete...\n");
    write_log("INFO", "Waiting for active connections to complete");
    
    int wait_time = 0;
    const int max_wait = 5; // Maximum 5 seconds wait
    
    while (get_active_connections() > 0 && wait_time < max_wait) {
        printf("[INFO] %d connections still active... (%d/%d seconds)\n", 
               get_active_connections(), wait_time + 1, max_wait);
        sleep(1);
        wait_time++;
    }
    
    if (get_active_connections() > 0) {
        printf("[WARN] Forcing shutdown with %d active connections.\n", 
               get_active_connections());
        write_log("WARN", "Forced shutdown with active connections");
    } else {
        printf("[INFO] All connections closed gracefully.\n");
        write_log("INFO", "All connections closed gracefully");
    }
    
    // Cleanup resources
    cleanup_server_globals();
    
    printf("[INFO] Server shutdown complete.\n");
    write_log("INFO", "Server shutdown complete");
    
    return 0;
}
