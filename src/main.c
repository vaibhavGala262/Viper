#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "network/epoll_handler.h"
#include "router.h"
#include "globals.h"
#include "log.h"

#define PORT 8080

static epoll_server_t* g_server = NULL;

void signal_handler(int sig) {
    if (g_server) {
        epoll_server_stop(g_server);
    }
    server_shutdown = 1;
}

static int create_server_socket(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) return -1;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address = {0};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, SOMAXCONN) < 0) {
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int main() {
    // Initialize
    init_server_globals();
    init_routes();

    // Setup signal handling
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);

    // Create server socket
    int server_fd = create_server_socket(PORT);
    if (server_fd == -1) {
        write_log("ERROR", "Failed to create server socket");
        exit(EXIT_FAILURE);
    }

    // Create epoll server
    g_server = epoll_server_create(server_fd);
    if (!g_server) {
        write_log("ERROR", "Failed to create epoll server");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Viper !! HTTP Server listening on port %d...\n", PORT);
   
    write_log("INFO", "Viper server started successfully");

    // Run server
    int result = epoll_server_run(g_server);

    // Cleanup
    epoll_server_destroy(g_server);
    close(server_fd);
    cleanup_server_globals();

    printf("Server Shutdown Gracefully.\n");
    write_log("INFO", "Server shutdown complete");

    return result;
}
