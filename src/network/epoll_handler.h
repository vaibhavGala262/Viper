#ifndef EPOLL_HANDLER_H
#define EPOLL_HANDLER_H

#define MAX_EVENTS 1000

typedef struct {
    int epoll_fd;
    int server_fd;
    int running;
} epoll_server_t;

epoll_server_t* epoll_server_create(int server_socket);
void epoll_server_destroy(epoll_server_t* server);
int epoll_server_run(epoll_server_t* server);
void epoll_server_stop(epoll_server_t* server);

#endif
