#include "api_handlers.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Actual handler implementations
ApiResponse get_user_handler(HttpRequest *req, RouteParam *params, int param_count) {
    const char *user_id = NULL;
    
    // Extract user ID from route parameters
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, "id") == 0) {
            user_id = params[i].value;
            break;
        }
    }
    
    ApiResponse response;
    asprintf(&response.content, 
             "{\"user_id\": \"%s\", \"name\": \"User %s\", \"email\": \"user%s@example.com\"}", 
             user_id, user_id, user_id);
    response.content_type = "application/json";
    response.status_code = 200;
    
    return response;
}


ApiResponse get_add(HttpRequest *req , RouteParam *params, int param_count){
    int num1 = 0, num2 = 0;
    
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, "num1") == 0) num1 = atoi(params[i].value);
        if (strcmp(params[i].key, "num2") == 0) num2 = atoi(params[i].value);
    }
    
    ApiResponse response;
    asprintf(&response.content, 
             "{\"num1\": %d, \"num2\": %d, \"sum\": %d, \"product\": %d}", 
             num1, num2, num1 + num2, num1 * num2);
    response.content_type = "application/json";
    response.status_code = 200;
    
    return response;
}

ApiResponse get_current_time_handler(HttpRequest *req, RouteParam *params, int param_count) {
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
    response.content_type = "application/json";
    response.status_code = 200;
    
    return response;
}

ApiResponse get_count_handler(HttpRequest *req, RouteParam *params, int param_count) {
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
             count_id, count_value);
    response.content_type = "application/json";
    response.status_code = 200;
    
    return response;
}

// Maping of names to functions (Registry)
HandlerFunc find_handler_function(const char *handler_name) {
    // Map handler names to actual function pointers
    if (strcmp(handler_name, "get_user") == 0) {
        return get_user_handler;
    }
    if (strcmp(handler_name, "get_current_time") == 0) {
        return get_current_time_handler;
    }
    if (strcmp(handler_name, "get_count") == 0) {
        return get_count_handler;
    }
    if (strcmp(handler_name, "get_user_post") == 0) {
        
        return NULL;
    }
    if (strcmp(handler_name, "get_product") == 0) {
        return NULL;
    }
    if(strcmp(handler_name , "get_add")==0){
        return get_add;
    }
    
    return NULL; // Handler not found
}

void free_api_response(ApiResponse *response) {
    if (response->content) {
        free(response->content);
        response->content = NULL;
    }
}
