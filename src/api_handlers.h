#ifndef API_HANDLERS_H
#define API_HANDLERS_H

#include "request.h"
#include "route_manager.h"

ApiResponse get_user_handler(HttpRequest *req, RouteParam *params, int param_count);
ApiResponse get_add(HttpRequest *req, RouteParam *params, int param_count);
ApiResponse get_current_time_handler(HttpRequest *req, RouteParam *params, int param_count);
ApiResponse get_count_handler(HttpRequest *req, RouteParam *params, int param_count);
ApiResponse get_sub(HttpRequest *req, RouteParam *params, int param_count);

ApiResponse create_user_handler(HttpRequest *req, RouteParam *params, int param_count);
ApiResponse update_user_handler(HttpRequest *req, RouteParam *params, int param_count);


HandlerFunc find_handler_function(const char *handler_name);
void free_api_response(ApiResponse *response);

#endif
