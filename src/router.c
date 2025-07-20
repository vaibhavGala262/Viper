#include <stdio.h>
#include <string.h>
#include "router.h"

// Global route manager
static RouteManager route_manager;



void init_routes(void) {
    init_route_manager(&route_manager);
    
    // API routes
    add_route(&route_manager, "/users/:id", ROUTE_TYPE_API, "get_user");
    add_route(&route_manager, "/users/:id/posts/:post_id", ROUTE_TYPE_API, "get_user_post");
    add_route(&route_manager, "/api/products/:category/:id", ROUTE_TYPE_API, "get_product");
    add_route(&route_manager, "/count/:id", ROUTE_TYPE_API, "get_count");
    add_route(&route_manager , "api/time" , ROUTE_TYPE_API , "get_current_time");
    add_route(&route_manager , "/api/calculate/:num1/:num2", ROUTE_TYPE_API  ,"get_add" );
    
    // STATIC routes 
    add_route(&route_manager, "/", ROUTE_TYPE_STATIC, "/html/index.html");
    add_route(&route_manager, "/trail", ROUTE_TYPE_STATIC, "/html/trail.html");
    add_route(&route_manager, "/css/style.css", ROUTE_TYPE_STATIC, "/css/style.css");
    add_route(&route_manager, "/images/roman", ROUTE_TYPE_STATIC, "/images/image4.jpg");
    add_route(&route_manager, "/downloads/result", ROUTE_TYPE_STATIC, "/downloads/result.pdf");
    
    // REDIRECT routes
    add_route(&route_manager, "/home", ROUTE_TYPE_REDIRECT, "/");
    add_route(&route_manager, "/index", ROUTE_TYPE_REDIRECT, "/");
    add_route(&route_manager, "/about", ROUTE_TYPE_REDIRECT, "/trail");
    
    printf("Routes initialized\n");
}


int check_route(const char *path, RouteParam *params, Route **matched_route) {
    return find_route(&route_manager, path, params, MAX_ROUTE_PARAMS, matched_route);
}



