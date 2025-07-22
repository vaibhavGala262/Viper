#include "api_handlers.h"
#include "route_manager.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Actual handler implementations
ApiResponse get_user_handler(HttpRequest *req, RouteParam *params, int param_count) {
    (void)req;  // Suppress unused parameter warning
    
    const char *user_id = NULL;
    
    // Extract user ID from route parameters
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, "id") == 0) {
            user_id = params[i].value;
            break;
        }
    }
    
    ApiResponse response;
    if (user_id) {
        asprintf(&response.content, 
                 "{\"user_id\": \"%s\", \"name\": \"User %s\", \"email\": \"user%s@example.com\"}", 
                 user_id, user_id, user_id);
        response.status_code = 200;
    } else {
        asprintf(&response.content, "{\"error\": \"User ID not provided\"}");
        response.status_code = 400;
    }
    response.content_type =strdup("application/json");
    
    return response;
}

ApiResponse get_add(HttpRequest *req, RouteParam *params, int param_count){
    (void)req;  // Suppress unused parameter warning
    
    int num1 = 0, num2 = 0;
    
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, "num1") == 0) num1 = atoi(params[i].value);
        if (strcmp(params[i].key, "num2") == 0) num2 = atoi(params[i].value);
    }
    
    ApiResponse response;
    asprintf(&response.content, 
             "{\"num1\": %d, \"num2\": %d, \"sum\": %d, \"product\": %d}", 
             num1, num2, num1 + num2, num1 * num2);
    response.content_type = strdup("application/json");
    response.status_code = 200;
    
    return response;
}

ApiResponse get_sub(HttpRequest *req, RouteParam *params, int param_count){
    (void)req;  // Suppress unused parameter warning
    
    int num1 = 0, num2 = 0;
    
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, "num1") == 0) num1 = atoi(params[i].value);
        if (strcmp(params[i].key, "num2") == 0) num2 = atoi(params[i].value);
    }
    
    ApiResponse response;
    asprintf(&response.content, 
             "{\"num1\": %d, \"num2\": %d, \"sub\": %d, \"division\": %d}", 
             num1, num2, num1 - num2, num1 % num2);
    response.content_type = strdup("application/json");
    response.status_code = 200;
    
    return response;
}
ApiResponse get_current_time_handler(HttpRequest *req, RouteParam *params, int param_count) {
    (void)req;          // Suppress unused parameter warnings
    (void)params;
    (void)param_count;
    
    time_t raw_time;
    struct tm *time_info;
    char time_buffer[64];
    
    time(&raw_time);
    time_info = localtime(&raw_time);
    strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", time_info);
    
    ApiResponse response;
    asprintf(&response.content, 
             "{\"current_time\": \"%s\", \"timestamp\": %ld}", 
             time_buffer, raw_time);
    response.content_type = strdup("application/json");
    response.status_code = 200;
    
    return response;
}

ApiResponse get_count_handler(HttpRequest *req, RouteParam *params, int param_count) {
    (void)req;  // Suppress unused parameter warning
    
    const char *count_id = NULL;
    
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, "id") == 0) {
            count_id = params[i].value;
            break;
        }
    }
    
    // Convert ID to number and return count
    int count_value = count_id ? atoi(count_id) * 10 : 0;
    
    ApiResponse response;
    asprintf(&response.content, 
             "{\"count_id\": \"%s\", \"count_value\": %d}", 
             count_id ? count_id : "null", count_value);
    response.content_type = strdup("application/json");
    response.status_code = 200;
    
    return response;
}

// Mapping of names to functions (Registry)
HandlerFunc find_handler_function(const char *handler_name) {
    if (strcmp(handler_name, "get_user") == 0) {
        return get_user_handler;
    }
    if (strcmp(handler_name, "get_current_time") == 0) {
        return get_current_time_handler;
    }
    if (strcmp(handler_name, "get_count") == 0) {
        return get_count_handler;
    }
    if (strcmp(handler_name, "get_add") == 0) {
        return get_add;
    }
    
    return NULL; // Handler not found
}

ApiResponse create_user_handler(HttpRequest *req, RouteParam *params, int param_count) {
    (void)params;
    (void)param_count;
    
    if (req->body) {
        printf("Body content: '%s'\n", req->body);
    }
    printf("Content-Type: '%s'\n", req->content_type);
    printf("===================================\n");
    ApiResponse response;
    
    // Check if it's a POST request with body
    if (strcmp(req->method, "POST") != 0 || !has_request_body(req)) {
        asprintf(&response.content, "{\"error\": \"POST request with body required\"}");
        response.status_code = 400;
        response.content_type = strdup("application/json");
        return response;
    }
    
    // Parse JSON body
    char name[64] = {0};
    char email[128] = {0};
    
    if (parse_json_body(req, "name", name, sizeof(name)) == 0 &&
        parse_json_body(req, "email", email, sizeof(email)) == 0) {
        
        // Process user creation (mock implementation can do complex operations based on data recieved example : store in database , call other apis etc)
        asprintf(&response.content, 
                 "{\"message\": \"User created\", \"name\": \"%s\", \"email\": \"%s\", \"id\": %d}", 
                 name, email, rand() % 1000);
        response.status_code = 201;
    } else {
        asprintf(&response.content, "{\"error\": \"Invalid JSON data\"}");
        response.status_code = 400;
    }
    
    response.content_type = strdup("application/json");
    return response;
}


ApiResponse update_user_handler(HttpRequest *req, RouteParam *params, int param_count) {
    (void)req;  // compiler dont give me error for not using this
    
    const char *user_id = NULL;
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, "id") == 0) {
            user_id = params[i].value;
            break;
        }
    }
    
    ApiResponse response;
    response.content_type =strdup("application/json");
    
    if (strcmp(req->method, "PUT") != 0 || !has_request_body(req)) {
        asprintf(&response.content, "{\"error\": \"PUT request with body required\"}");
        response.status_code = 400;
        return response;
    }
    
    // Parse JSON for updates
    char name[64] = {0};
    char email[128] = {0};
    
    parse_json_body(req, "name", name, sizeof(name));
    parse_json_body(req, "email", email, sizeof(email));
    
    asprintf(&response.content, 
             "{\"message\": \"User updated\", \"id\": \"%s\", \"name\": \"%s\", \"email\": \"%s\"}", 
             user_id, name, email);
    response.status_code = 200;
    
    return response;
}

void free_api_response(ApiResponse* response) {
    if (response) {
        if (response->content) {
            free(response->content);
            response->content = NULL;
        }
        if (response->content_type) {
            free(response->content_type);
            response->content_type = NULL;
        }
    }
}
