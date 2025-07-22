#include "route_manager.h"
#include "middleware.h"

#include <string.h>
#include <stdio.h>

void init_route_manager(RouteManager *rm) {
    rm->count = 0;
    memset(rm->routes, 0, sizeof(rm->routes));
}

int add_api_route_with_method(RouteManager *rm, const char *pattern, const char *method, HandlerFunc handler) {
    if (rm->count >= MAX_ROUTES) return -1;
    
    Route *route = &rm->routes[rm->count];
    strcpy(route->pattern, pattern);
    strcpy(route->method, method);
    route->type = ROUTE_TYPE_API;
    route->handler.api_handler = handler;
    route->middleware = NULL;  // No middleware for regular routes
    
    rm->count++;
    return 0;
}
int find_route(RouteManager *rm, const char *path, const char *method, RouteParam *params, int max_params, Route **matched_route) {
    for (int i = 0; i < rm->count; i++) {
        int param_count = match_route(rm->routes[i].pattern, path, params, max_params);
        if (param_count >= 0) {
            // Check if HTTP method matches or route accepts all methods (empty method field)
            if (strlen(rm->routes[i].method) == 0 || strcasecmp(rm->routes[i].method, method) == 0) {
                *matched_route = &rm->routes[i];
                return param_count;
            }
        }
    }
    return -1; // No matching route found
}
int add_static_route(RouteManager *rm, const char *pattern, const char *file_path) {
    if (rm->count >= MAX_ROUTES) return -1;
    
    Route *route = &rm->routes[rm->count];
    strcpy(route->pattern, pattern);
    route->type = ROUTE_TYPE_STATIC;
    strcpy(route->handler.static_path, file_path);
    snprintf(route->description, sizeof(route->description), "Static: %s -> %s", pattern, file_path);
    
    rm->count++;
    return 0;
}

int add_redirect_route(RouteManager *rm, const char *pattern, const char *redirect_url) {
    if (rm->count >= MAX_ROUTES) return -1;
    
    Route *route = &rm->routes[rm->count];
    strcpy(route->pattern, pattern);
    route->type = ROUTE_TYPE_REDIRECT;
    strcpy(route->handler.redirect_url, redirect_url);
    snprintf(route->description, sizeof(route->description), "Redirect: %s -> %s", pattern, redirect_url);
    
    rm->count++;
    return 0;
}


int add_api_route_with_middleware(RouteManager *rm, const char *pattern, const char *method, 
                                 HandlerFunc handler, middleware_chain_t* middleware) {
    if (rm->count >= MAX_ROUTES) return -1;
    
  
    
    Route *route = &rm->routes[rm->count];
    strcpy(route->pattern, pattern);
    strcpy(route->method, method);
    route->type = ROUTE_TYPE_API;
    route->handler.api_handler = handler;
    route->middleware = middleware; 
    
    rm->count++;
    return 0;
}


