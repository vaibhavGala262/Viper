#include "middleware.h"
#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

middleware_chain_t* create_middleware_chain(void) {
   
    middleware_chain_t* chain = malloc(sizeof(middleware_chain_t));
    if (!chain) {
        printf("DEBUG: Failed to allocate middleware chain\n");
        return NULL;
    }
    
    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
    
    
    return chain;
}

void add_middleware(middleware_chain_t* chain, middleware_fn handler) {
   
    
    if (!chain || !handler) {
        printf("DEBUG: NULL chain or handler in add_middleware\n");
        return;
    }
    
    middleware_node_t* node = malloc(sizeof(middleware_node_t));
    if (!node) {
        printf("DEBUG: Failed to allocate middleware node\n");
        return;
    }
    
    node->handler = handler;
    node->next = NULL;
    
    if (chain->tail) {
        chain->tail->next = node;
        chain->tail = node;
    } else {
        chain->head = chain->tail = node;
    }
    
    chain->count++;
    
}



middleware_result_t execute_middleware_chain(middleware_chain_t* chain, HttpRequest *req, ApiResponse *res) {
    
    
    if (!chain) {
        printf("DEBUG: Chain is NULL\n");
        return MIDDLEWARE_CONTINUE;
    }
    
   
    
    if (!chain->head) {
        printf("DEBUG: Chain head is NULL\n");
        return MIDDLEWARE_CONTINUE;
    }
    
    if (!req) {
        printf("DEBUG: Request is NULL\n");
        return MIDDLEWARE_ERROR;
    }
    
    if (!res) {
        printf("DEBUG: Response is NULL\n");
        return MIDDLEWARE_ERROR;
    }
    
    
    
    middleware_node_t* current = chain->head;
    int middleware_index = 0;
    
    while (current) {
       
        
        if (!current->handler) {
            printf("DEBUG: Handler is NULL for middleware %d\n", middleware_index);
            return MIDDLEWARE_ERROR;
        }
        
        
        middleware_result_t result = current->handler(req, res);
        
        printf("DEBUG: Handler returned: %d\n", result);
        fflush(stdout);
        
        if (result != MIDDLEWARE_CONTINUE) {
            return result;
        }
        
        current = current->next;
        middleware_index++;
      
    }
    
    printf("DEBUG: Middleware chain execution completed successfully\n");
    return MIDDLEWARE_CONTINUE;
}

void free_middleware_chain(middleware_chain_t* chain) {
    if (!chain) return;
    
    middleware_node_t* current = chain->head;
    while (current) {
        middleware_node_t* next = current->next;
        free(current);
        current = next;
    }
    
    free(chain);
}




// Adding middleware below
middleware_result_t cors_middleware(HttpRequest *req, ApiResponse *res) {
  
    if (!req || !res) {
        printf("DEBUG: CORS middleware - NULL req or res\n");
        return MIDDLEWARE_ERROR;
    }
    
    
    
    if (strcmp(req->method, "OPTIONS") == 0) {
        res->status_code = 200;
        res->content = strdup("OK");
        res->content_type = strdup("text/plain");
        printf("DEBUG: CORS middleware - handling OPTIONS request\n");
        return MIDDLEWARE_STOP;
    }
    
   
    return MIDDLEWARE_CONTINUE;
}

middleware_result_t logging_middleware(HttpRequest *req, ApiResponse *res) {
    
    if (!req) {
        printf("DEBUG: Logging middleware - NULL request\n");
        return MIDDLEWARE_ERROR;
    }
    
    (void)res;

    
    return MIDDLEWARE_CONTINUE;
}


middleware_result_t strict_cors_middleware(HttpRequest *req, ApiResponse *res) {
    // Get Origin header
    const char* origin = NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, "Origin") == 0) {
            origin = req->headers[i].value;
            break;
        }
    }
    
    // Block requests from unauthorized origins
    if (origin && strcmp(origin, "https://allowed-domain.com") != 0) {
        res->status_code = 403;
        res->content = strdup("{\"error\": \"CORS: Origin not allowed\"}");
        res->content_type = strdup("application/json");
        return MIDDLEWARE_STOP;  
    }
    
    return MIDDLEWARE_CONTINUE;
}


middleware_result_t auth_middleware(HttpRequest *req, ApiResponse *res) {
    write_log("INFO", "AUTH middleware: Checking authorization");
    
    // Look for Authorization header
    const char* auth_header = NULL;
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, "Authorization") == 0) {
            auth_header = req->headers[i].value;
            break;
        }
    }
    
    if (!auth_header) {
        write_log("WARN", "AUTH middleware: No Authorization header found");
        res->status_code = 401;
        res->content = strdup("{\"error\": \"Authorization header required\"}");
        res->content_type = strdup("application/json");
        return MIDDLEWARE_STOP;
    }
    

    // Simple token validation which can be improved
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        write_log("WARN", "AUTH middleware: Invalid Authorization format");
        res->status_code = 401;
        res->content = strdup("{\"error\": \"Invalid authorization format. Use: Bearer <token>\"}");
        res->content_type = strdup("application/json");
        return MIDDLEWARE_STOP;
    }
    
    const char* token = auth_header + 7; // Skip "Bearer "
    
    // Simple token validation - you can replace this with your actual validation
    if (strcmp(token, "valid_token_123") != 0) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "AUTH middleware: Invalid token: %.20s...", token);
        write_log("WARN", log_msg);
        res->status_code = 401;
        res->content = strdup("{\"error\": \"Invalid or expired token\"}");
        res->content_type = strdup("application/json");
        return MIDDLEWARE_STOP;
    }
    
    write_log("INFO", "AUTH middleware: Authorization successful");
    return MIDDLEWARE_CONTINUE;
}

// Rate limiting middleware
middleware_result_t rate_limit_middleware(HttpRequest *req, ApiResponse *res) {
    
    (void)req;
    static int request_count = 0;
    static time_t last_reset = 0;
    
    time_t now = time(NULL);
    
    // Reset counter every minute
    if (now - last_reset > 60) {
        request_count = 0;
        last_reset = now;
    }
    
    request_count++;
    
    // Limit to 100 requests per minute (example)
    if (request_count > 100) {
        char log_msg[256];
        snprintf(log_msg, sizeof(log_msg), "RATE LIMIT: Request %d exceeded limit", request_count);
        write_log("WARN", log_msg);
        
        res->status_code = 429;
        res->content = strdup("{\"error\": \"Too Many Requests\"}");
        res->content_type = strdup("application/json");
        return MIDDLEWARE_STOP;
    }
    
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "RATE LIMIT: Request %d/100", request_count);
    write_log("INFO", log_msg);
    
    return MIDDLEWARE_CONTINUE;
}