#include "route_manager.h"
#include <stdio.h>
#include <string.h>

void init_route_manager(RouteManager *rm) {
    if (!rm) return;
    rm->count = 0;
}

int add_route(RouteManager *rm, const char *pattern, RouteType type, const char *handler_name) {
    if (!rm || !pattern || rm->count >= MAX_ROUTES) return -1;
    
    strncpy(rm->routes[rm->count].pattern, pattern, sizeof(rm->routes[rm->count].pattern) - 1);
    rm->routes[rm->count].pattern[sizeof(rm->routes[rm->count].pattern) - 1] = '\0';
    
    rm->routes[rm->count].type = type;
    
    if (handler_name) {
        strncpy(rm->routes[rm->count].handler_name, handler_name, sizeof(rm->routes[rm->count].handler_name) - 1);
        rm->routes[rm->count].handler_name[sizeof(rm->routes[rm->count].handler_name) - 1] = '\0';
    } else {
        rm->routes[rm->count].handler_name[0] = '\0';
    }
    
    return rm->count++;
}

int find_route(RouteManager *rm, const char *path, RouteParam *params, int max_params, Route **matched_route) {
    if (!rm || !path || !params) return -1;
    
    for (int i = 0; i < rm->count; i++) {
        int param_count = match_route(rm->routes[i].pattern, path, params, max_params);
        if (param_count >= 0) {
            if (matched_route) {
                *matched_route = &rm->routes[i];
            }
            return param_count;
        }
    }
    
    if (matched_route) {
        *matched_route = NULL;
    }
    return -1;
}

void print_routes(const RouteManager *rm) {
    if (!rm) return;
    
    printf("Registered Routes (%d):\n", rm->count);
    for (int i = 0; i < rm->count; i++) {
        const char *type_str = "";
        switch (rm->routes[i].type) {
            case ROUTE_TYPE_API: type_str = "API"; break;
            case ROUTE_TYPE_STATIC: type_str = "STATIC"; break;
            case ROUTE_TYPE_REDIRECT: type_str = "REDIRECT"; break;
        }
        printf("  [%s] %s -> %s\n", type_str, rm->routes[i].pattern, rm->routes[i].handler_name);
    }
}
