#include <stdio.h>
#include <string.h>

#include "router.h"
#include "api_handlers.h"  
#include "middleware.h"

// Global route manager
static RouteManager route_manager;

void init_routes(void) {
    init_route_manager(&route_manager);
    middleware_chain_t* api_middleware = create_middleware_chain();
    
    add_middleware(api_middleware, strict_cors_middleware);
    add_middleware(api_middleware, logging_middleware);
    add_middleware(api_middleware, auth_middleware); 
    
    // Clean API route registration - direct function pointers!
    add_api_route_with_method(&route_manager, "/users/:id", "GET", get_user_handler);
    add_api_route_with_method(&route_manager, "/count/:id","GET", get_count_handler);
    add_api_route_with_method(&route_manager, "/api/time","GET", get_current_time_handler);
    add_api_route_with_method(&route_manager, "/api/calculate/:num1/:num2","GET", get_add);
    add_api_route_with_method(&route_manager, "/api/sub/:num1/:num2","GET", get_sub);

    add_api_route_with_method(&route_manager, "/users", "POST",create_user_handler);
    add_api_route_with_method(&route_manager, "/users/:id","PUT", update_user_handler);

    add_api_route_with_middleware(&route_manager, "/api/users", "GET", get_user_handler, api_middleware);
    add_api_route_with_middleware(&route_manager, "/api/protected/data", "GET", get_current_time_handler,api_middleware);

    
    // Static routes - no change in complexity
    add_static_route(&route_manager, "/", "/html/index.html");
    add_static_route(&route_manager, "/trail", "/html/trail.html");
    add_static_route(&route_manager, "/css/style.css", "/css/style.css");
    add_static_route(&route_manager, "/images/roman", "/images/image4.jpg");
    add_static_route(&route_manager, "/downloads/result", "/downloads/result.pdf");
    
    // Redirect routes
    add_redirect_route(&route_manager, "/home", "/");
    add_redirect_route(&route_manager, "/index", "/");
    add_redirect_route(&route_manager, "/about", "/trail");
    
   
}

int check_route_with_method(const char *path, const char *method, RouteParam *params, Route **matched_route) {
    return find_route(&route_manager, path, method, params, MAX_ROUTE_PARAMS, matched_route);
}

