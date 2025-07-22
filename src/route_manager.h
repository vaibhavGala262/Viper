#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include "pattern_matching.h"
#include "request.h"

#define MAX_ROUTES 32

// Forward declarations
typedef struct middleware_chain middleware_chain_t;



typedef struct {
    char *content;
    char *content_type;
    int status_code;
} ApiResponse;

// Handler function type
typedef ApiResponse (*HandlerFunc)(HttpRequest *req, RouteParam *params, int param_count);

typedef enum {
    ROUTE_TYPE_API,
    ROUTE_TYPE_STATIC,
    ROUTE_TYPE_REDIRECT
} RouteType;

typedef struct {
    char pattern[256];
    RouteType type;
    char method[16];   
    // Use union to store different handler types efficiently
    union {
        HandlerFunc api_handler;      // Direct function pointer for API routes
        char static_path[256];        // File path for static routes
        char redirect_url[256];       // Redirect URL for redirect routes
    } handler;
    middleware_chain_t* middleware; 
    char description[128];          
} Route;

typedef struct {
    Route routes[MAX_ROUTES];
    int count;
} RouteManager;

// Core functions
void init_route_manager(RouteManager *rm);

// Type-specific route registration functions
int add_api_route_with_method(RouteManager *rm, const char *pattern, const char *method, HandlerFunc handler);
int add_static_route(RouteManager *rm, const char *pattern, const char *file_path);
int add_redirect_route(RouteManager *rm, const char *pattern, const char *redirect_url);

int add_api_route_with_middleware(RouteManager *rm, const char *pattern, const char *method, 
                                 HandlerFunc handler, middleware_chain_t* middleware); // adding middleware
// Route finding
int find_route(RouteManager *rm, const char *path, const char *method, RouteParam *params, int max_params, Route **matched_route);

#endif
