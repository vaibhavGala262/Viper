#include <stdio.h>
#include <string.h>
#include "router.h"

// Global route manager
static RouteManager route_manager;

void init_routes(void) {
    init_route_manager(&route_manager);
    
    // Add your API routes here
    add_route(&route_manager, "/users/:id", ROUTE_TYPE_API, "get_user");
    add_route(&route_manager, "/users/:id/posts/:post_id", ROUTE_TYPE_API, "get_user_post");
    add_route(&route_manager, "/api/products/:category/:id", ROUTE_TYPE_API, "get_product");
    add_route(&route_manager, "/count/:id", ROUTE_TYPE_API, "get_count");

    
    printf("Routes initialized\n");
}

int check_api_route(const char *path, RouteParam *params) {
    Route *matched_route = NULL;
    return find_route(&route_manager, path, params, MAX_ROUTE_PARAMS, &matched_route);
}

// Your existing function - UNCHANGED
void resolve_path(char *path) {
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/html/index.html");
    } else if (strcmp(path, "/trail") == 0) {
        strcpy(path, "/html/trail.html");
    } else if (strcmp(path, "/css/style.css") == 0) {
        strcpy(path, "/css/style.css");
    } else if (strcmp(path, "/images/roman") == 0) {
        strcpy(path, "/images/image4.jpg");
    } else if(strcmp(path , "/downloads/result")==0){
        strcpy(path , "/downloads/result.pdf");
    }
    // else leave it unchanged
}