#include "globals.h"
#include "log.h"
#include <stdio.h>
#include <unistd.h>

// Global variable definitions
volatile sig_atomic_t server_shutdown = 0;
int server_socket = -1;


pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;
int active_connections = 0;


void signal_handler(int signum) {
    switch(signum) {
        case SIGINT:
            printf("\n[INFO] Received SIGINT (Ctrl+C). Initiating graceful shutdown...\n");
            write_log("INFO", "Received SIGINT - initiating graceful shutdown");
            break;
        case SIGTERM:
            printf("\n[INFO] Received SIGTERM. Initiating graceful shutdown...\n");
            write_log("INFO", "Received SIGTERM - initiating graceful shutdown");
            break;
        default:
            printf("\n[INFO] Received signal %d. Initiating graceful shutdown...\n", signum);
            write_log("INFO", "Received signal - initiating graceful shutdown");
            break;
    }
    
    server_shutdown = 1;
    
    // Close server socket to stop accepting new connections
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
        write_log("INFO", "Server socket closed for graceful shutdown");
    }
}

void increment_connections(void) {
    pthread_mutex_lock(&connection_mutex);
    active_connections++;
    pthread_mutex_unlock(&connection_mutex);
}

void decrement_connections(void) {
    pthread_mutex_lock(&connection_mutex);
    active_connections--;
    pthread_mutex_unlock(&connection_mutex);
}

int get_active_connections(void) {
    pthread_mutex_lock(&connection_mutex);
    int count = active_connections;
    pthread_mutex_unlock(&connection_mutex);
    return count;
}

void init_server_globals(void) {
    server_shutdown = 0;
    server_socket = -1;
    active_connections = 0;
    write_log("INFO", "Server globals initialized");
}

void cleanup_server_globals(void) {
    if (server_socket != -1) {
        close(server_socket);
        server_socket = -1;
    }
    pthread_mutex_destroy(&connection_mutex);
    write_log("INFO", "Server globals cleaned up");
}
