#include "epoll_handler.h"
#include "../core/connection.h"
#include "../handlers/route_dispatcher.h"
#include "../log.h"
#include "../globals.h"
#include <sys/epoll.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdio.h>

static int make_socket_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void handle_new_connection(epoll_server_t* server) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int client_fd;

    printf("DEBUG: handle_new_connection called\n");
    fflush(stdout);

    while ((client_fd = accept(server->server_fd, 
                              (struct sockaddr*)&client_addr, 
                              &client_len)) != -1) {
        
        printf("DEBUG: Accepted client_fd = %d\n", client_fd);
        fflush(stdout);
        
        if (make_socket_nonblocking(client_fd) == -1) {
            printf("DEBUG: Failed to make socket non-blocking\n");
            close(client_fd);
            continue;
        }
        printf("DEBUG: Made socket non-blocking\n");

        connection_t* conn = connection_create(client_fd);
        if (!conn) {
            printf("DEBUG: Failed to create connection\n");
            close(client_fd);
            continue;
        }
        printf("DEBUG: Created connection object\n");

        // IMPORTANT: Use level-triggered mode for now (remove EPOLLET)
        struct epoll_event ev;
        ev.events = EPOLLIN;  // Remove EPOLLET for easier debugging
        ev.data.fd = client_fd;
        
        printf("DEBUG: Registering fd %d with events 0x%x\n", client_fd, ev.events);
        fflush(stdout);
        
        if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            printf("DEBUG: epoll_ctl failed: %s\n", strerror(errno));
            fflush(stdout);
            connection_destroy(conn);
            continue;
        } else {
            printf("DEBUG: epoll_ctl successful for fd %d\n", client_fd);
            fflush(stdout);
        }

        increment_connections();

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        
        printf("DEBUG: Connection from %s (fd=%d) setup complete\n", client_ip, client_fd);
        fflush(stdout);
    }

    if (errno != EAGAIN && errno != EWOULDBLOCK) {
        printf("DEBUG: accept() error: %s\n", strerror(errno));
        fflush(stdout);
    }
}

epoll_server_t* epoll_server_create(int server_socket) {
    epoll_server_t* server = malloc(sizeof(epoll_server_t));
    if (!server) return NULL;

    server->epoll_fd = epoll_create1(0);
    if (server->epoll_fd == -1) {
        free(server);
        return NULL;
    }

    server->server_fd = server_socket;
    server->running = 1;

    // Make server socket non-blocking
    make_socket_nonblocking(server_socket);

    // Add server socket to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = server_socket;
    
    if (epoll_ctl(server->epoll_fd, EPOLL_CTL_ADD, server_socket, &ev) == -1) {
        close(server->epoll_fd);
        free(server);
        return NULL;
    }

    return server;
}

void epoll_server_destroy(epoll_server_t* server) {
    if (!server) return;
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (g_connections[i]) {
            connection_destroy(g_connections[i]);
        }
    }
    
    close(server->epoll_fd);
    free(server);
}

int epoll_server_run(epoll_server_t* server) {
    struct epoll_event events[MAX_EVENTS];
    extern volatile int server_shutdown;
    
    printf("DEBUG: Starting epoll event loop\n");
    fflush(stdout);
    
    while (server->running && !server_shutdown) {
    
        int nfds = epoll_wait(server->epoll_fd, events, MAX_EVENTS, 1000);
        
        if (server_shutdown) {
            printf("DEBUG: Shutdown flag detected, breaking loop\n");
            break;
        }
        
       
        
        if (nfds == -1) {
            if (errno == EINTR) {
                printf("DEBUG: epoll_wait interrupted by signal\n");
                continue;
            }
            printf("DEBUG: epoll_wait error: %s\n", strerror(errno));
            write_log("ERROR", "epoll_wait failed");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            printf("DEBUG: Event %d: fd=%d, events=0x%x\n", i, fd, events[i].events);
            fflush(stdout);

            if (fd == server->server_fd) {
                printf("DEBUG: Server socket event - new connection\n");
                fflush(stdout);
                handle_new_connection(server);
            } else {
                connection_t* conn = g_connections[fd];
                printf("DEBUG: Client socket event for fd %d, conn=%p\n", fd, (void*)conn);
                fflush(stdout);
                
                if (!conn) {
                    printf("DEBUG: No connection found for fd %d\n", fd);
                    continue;
                }

                if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                    printf("DEBUG: Error/hangup for fd %d\n", fd);
                    fflush(stdout);
                    decrement_connections();
                    connection_destroy(conn);
                } else if (events[i].events & EPOLLIN) {
                    printf("DEBUG: EPOLLIN detected for fd %d - reading data\n", fd);
                    fflush(stdout);
                    
                    int result = connection_read_data(conn);
                    printf("DEBUG: connection_read_data returned %d\n", result);
                    fflush(stdout);
                    
                    if (result <= 0) {
                        printf("DEBUG: Read failed, closing connection fd %d\n", fd);
                        decrement_connections();
                        connection_destroy(conn);
                        continue;
                    }
                    
                    if (connection_is_request_complete(conn)) {
                        printf("DEBUG: Request complete, dispatching to handler\n");
                        fflush(stdout);
                        
                        // Create a simple response instead of complex routing for now
                        
                        
                        route_dispatcher_handle_request(conn);
                        // Switch to write mode
                        struct epoll_event ev;
                        ev.events = EPOLLOUT;
                        ev.data.fd = fd;
                        epoll_ctl(server->epoll_fd, EPOLL_CTL_MOD, fd, &ev);
                    }
                } else if (events[i].events & EPOLLOUT) {
                    printf("DEBUG: EPOLLOUT detected for fd %d - writing response\n", fd);
                    fflush(stdout);
                    
                    int result = connection_write_data(conn);
                    printf("DEBUG: connection_write_data returned %d\n", result);
                    fflush(stdout);
                    
                    if (result != 0) {
                        printf("DEBUG: Write complete, closing connection fd %d\n", fd);
                        decrement_connections();
                        connection_destroy(conn);
                    }
                }
            }
        }
    }
    
    printf("DEBUG: Exited epoll loop\n");
    return 0;
}

void epoll_server_stop(epoll_server_t* server) {
    if (server) {
        server->running = 0;
    }
}
