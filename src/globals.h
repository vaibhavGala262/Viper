#ifndef GLOBALS_H
#define GLOBALS_H

#include <signal.h>
#include <stdbool.h>
#include <pthread.h>

// Global shutdown flag - thread-safe atomic type
extern volatile sig_atomic_t server_shutdown;

// Global server socket for cleanup
extern int server_socket;

// Thread-safe connection tracking
extern pthread_mutex_t connection_mutex;
extern int active_connections;



// Connection tracking functions
void increment_connections(void);
void decrement_connections(void);
int get_active_connections(void);

// Server initialization
void init_server_globals(void);
void cleanup_server_globals(void);

#endif // GLOBALS_H
