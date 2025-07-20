#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include "request.h"
#include "route_manager.h"

// Response structure for API handlers
typedef struct {
    char *content;
    char *content_type;
    int status_code;
} ApiResponse;

// Handler function signature - ALL handlers must follow this pattern
typedef ApiResponse (*HandlerFunc)(HttpRequest *req, RouteParam *params, int param_count);

// Function to map handler names to actual functions
HandlerFunc find_handler_function(const char *handler_name);

// Helper to free API response memory
void free_api_response(ApiResponse *response);

#endif
