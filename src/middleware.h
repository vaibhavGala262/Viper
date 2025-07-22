#ifndef MIDDLEWARE_H
#define MIDDLEWARE_H

#include "request.h"
#include "route_manager.h"

typedef enum {
    MIDDLEWARE_CONTINUE,
    MIDDLEWARE_STOP,
    MIDDLEWARE_ERROR
} middleware_result_t;

// Middleware function signature - matches your existing ApiResponse pattern
typedef middleware_result_t (*middleware_fn)(HttpRequest *req, ApiResponse *res);

typedef struct middleware_node {
    middleware_fn handler;
    struct middleware_node* next;
} middleware_node_t;

typedef struct middleware_chain {
    middleware_node_t* head;
    middleware_node_t* tail;
    int count;
} middleware_chain_t;

// Core middleware functions
middleware_chain_t* create_middleware_chain(void);
void add_middleware(middleware_chain_t* chain, middleware_fn handler);
middleware_result_t execute_middleware_chain(middleware_chain_t* chain, HttpRequest *req, ApiResponse *res);
void free_middleware_chain(middleware_chain_t* chain);

// Common middleware implementations
middleware_result_t cors_middleware(HttpRequest *req, ApiResponse *res);
middleware_result_t logging_middleware(HttpRequest *req, ApiResponse *res);
middleware_result_t auth_middleware(HttpRequest *req, ApiResponse *res);
middleware_result_t strict_cors_middleware(HttpRequest *req, ApiResponse *res);

#endif
