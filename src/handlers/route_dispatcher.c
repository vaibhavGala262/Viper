#include "route_dispatcher.h"
#include "../request.h"
#include "../middleware.h"
#include "../router.h"
#include "../api_handlers.h"
#include "../log.h"
#include "../mime.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


// Most logic is from client.c

int route_dispatcher_handle_request(connection_t *conn)
{
    // Parse HTTP request
    conn->parsed_request = malloc(sizeof(HttpRequest));
    if (!conn->parsed_request)
    {
        write_log("ERROR", "Failed to allocate request");
        return -1;
    }
    memset(conn->parsed_request, 0, sizeof(HttpRequest)); // Initialize to zero

    if (parse_http_request(conn->buffer, conn->parsed_request) != 0)
    {
        const char *bad_request =
            "HTTP/1.1 400 Bad Request\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 15\r\n"
            "Connection: close\r\n"
            "\r\n"
            "Malformed request";

        conn->response = strdup(bad_request);
        conn->response_len = strlen(bad_request);
        conn->state = CONN_WRITING;
        return 0;
    }

    // Log parsed request details (matching your original client.c style)
    char method_log[1200];
    snprintf(method_log, sizeof(method_log), "Method: %s, Path: %s",
             conn->parsed_request->method, conn->parsed_request->path);
    write_log("INFO", method_log);

    // UNIFIED ROUTE HANDLING (exactly like your client.c)
    RouteParam route_params[MAX_ROUTE_PARAMS];
    Route *matched_route = NULL;
    int param_count = check_route_with_method(conn->parsed_request->path,
                                              conn->parsed_request->method,
                                              route_params, &matched_route);

    if (param_count >= 0 && matched_route)
    {
        printf("DEBUG: Route matched successfully - pattern: %s, type: %d\n",
               matched_route->pattern, matched_route->type);
        // Log the matched route
        char route_log[512];
        snprintf(route_log, sizeof(route_log), "Matched route: %s (type: %d)",
                 matched_route->pattern, matched_route->type);
        write_log("INFO", route_log);

        printf("DEBUG: About to enter switch statement for route type %d\n", matched_route->type);
        switch (matched_route->type)
        {
        case ROUTE_TYPE_API:
        {
            printf("DEBUG: Entering API route handling\n");
            fflush(stdout);

            // Log route parameters
            printf("DEBUG: Processing %d route parameters\n", param_count);
            fflush(stdout);
            // Log route parameters
            for (int i = 0; i < param_count; i++)
            {
                char param_log[400];
                snprintf(param_log, sizeof(param_log), "Route Param - %s: %s",
                         route_params[i].key, route_params[i].value);
                write_log("INFO", param_log);
            }

            printf("DEBUG: Checking if handler exists: %p\n", (void *)matched_route->handler.api_handler);
            fflush(stdout);

            if (matched_route->handler.api_handler)
            {
                ApiResponse api_response = {0}; // Initialize to zero
                printf("DEBUG: ApiResponse initialized\n");
                fflush(stdout);

                // Check if middleware exists
                printf("DEBUG: Checking middleware: %p\n", (void *)matched_route->middleware);
                fflush(stdout);
                // Execute middleware chain if present
                if (matched_route->middleware)
                {
                    printf("DEBUG: Middleware exists - executing chain\n");
                    fflush(stdout);
                    char middleware_log[300];
                    snprintf(middleware_log, sizeof(middleware_log),
                             "Executing middleware chain for route: %s", matched_route->pattern);
                    write_log("INFO", middleware_log);

                    middleware_result_t middleware_result = execute_middleware_chain(
                        matched_route->middleware, conn->parsed_request, &api_response);

                    if (middleware_result == MIDDLEWARE_STOP)
                    {
                        write_log("INFO", "Middleware stopped execution - sending middleware response");

                        // Build response from middleware
                        char response_header[800];
                        size_t content_len = api_response.content ? strlen(api_response.content) : 0;
                        snprintf(response_header, sizeof(response_header),
                                 "HTTP/1.1 %d %s\r\n"
                                 "Content-Type: %s\r\n"
                                 "Content-Length: %zu\r\n"
                                 "Access-Control-Allow-Origin: *\r\n"
                                 "Connection: close\r\n"
                                 "\r\n",
                                 api_response.status_code,
                                 api_response.status_code == 200 ? "OK" : api_response.status_code == 401 ? "Unauthorized"
                                                                      : api_response.status_code == 400   ? "Bad Request"
                                                                                                          : "Error",
                                 api_response.content_type ? api_response.content_type : "text/plain",
                                 content_len);

                        size_t header_len = strlen(response_header);
                        size_t total_len = header_len + content_len;

                        conn->response = malloc(total_len + 1);
                        if (conn->response)
                        {
                            memcpy(conn->response, response_header, header_len);
                            if (api_response.content && content_len > 0)
                            {
                                memcpy(conn->response + header_len, api_response.content, content_len);
                            }
                            conn->response[total_len] = '\0';
                            conn->response_len = total_len;
                        }

                        // Clean up middleware response CAREFULLY
                        if (api_response.content && api_response.content != conn->response)
                        {
                            free(api_response.content);
                        }
                        if (api_response.content_type)
                        {
                            free(api_response.content_type);
                        }

                        conn->state = CONN_WRITING;
                        return 0;
                    }

                    if (middleware_result == MIDDLEWARE_ERROR)
                    {
                        write_log("ERROR", "Middleware execution error");

                        // Clean up any existing response from middleware
                        if (api_response.content)
                        {
                            free(api_response.content);
                        }
                        if (api_response.content_type)
                        {
                            free(api_response.content_type);
                        }

                        // Set error response
                        const char *error_response =
                            "HTTP/1.1 500 Internal Server Error\r\n"
                            "Content-Type: text/plain\r\n"
                            "Content-Length: 21\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "Internal Server Error";

                        conn->response = strdup(error_response);
                        conn->response_len = strlen(error_response);
                        conn->state = CONN_WRITING;
                        return 0;
                    }

                    write_log("INFO", "Middleware chain executed successfully - continuing to route handler");
                }
                printf("DEBUG: About to call handler: %p\n", (void *)matched_route->handler.api_handler);
                fflush(stdout);
                // Call the handler function directly (your original logic)
                api_response = matched_route->handler.api_handler(conn->parsed_request, route_params, param_count);

                printf("DEBUG: Handler call completed\n");
                printf("DEBUG: Response status: %d\n", api_response.status_code);
                printf("DEBUG: Response content: %p\n", (void *)api_response.content);
                printf("DEBUG: Response content_type: %p\n", (void *)api_response.content_type);
                fflush(stdout);


                if (api_response.content) {
    printf("DEBUG: Content string: '%.50s'\n", api_response.content);
    printf("DEBUG: Content length: %zu\n", strlen(api_response.content));
}
if (api_response.content_type) {
    printf("DEBUG: Content-type string: '%.50s'\n", api_response.content_type);
    printf("DEBUG: Content-type length: %zu\n", strlen(api_response.content_type));
}
fflush(stdout);
                // Build HTTP response
                if (api_response.content && api_response.content_type)
                {

                    char response_header[800];
                    size_t content_len = strlen(api_response.content);
                    snprintf(response_header, sizeof(response_header),
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Type: %s\r\n"
                             "Content-Length: %zu\r\n"
                             "Access-Control-Allow-Origin: *\r\n"
                             "Connection: close\r\n"
                             "\r\n",
                             api_response.status_code,
                             api_response.status_code == 200 ? "OK" : api_response.status_code == 400 ? "Bad Request"
                                                                  : api_response.status_code == 404   ? "Not Found"
                                                                                                      : "Error",
                             api_response.content_type,
                             content_len);

                    size_t header_len = strlen(response_header);
                    size_t total_len = header_len + content_len;

                    conn->response = malloc(total_len + 1);
                    if (conn->response)
                    {
                        memcpy(conn->response, response_header, header_len);
                        memcpy(conn->response + header_len, api_response.content, content_len);
                        conn->response[total_len] = '\0';
                        conn->response_len = total_len;
                    }

                    // Log success
                    char success_log[300];
                    snprintf(success_log, sizeof(success_log),
                             "API handler executed successfully, status: %d",
                             api_response.status_code);
                    write_log("INFO", success_log);

                    // Clean up API response - DON'T double free!
                    if (api_response.content)
                    {
                        free(api_response.content);
                    }
                    if (api_response.content_type)
                    {
                        free(api_response.content_type);
                    }
                }
                else
                {
                    // Handle case where handler didn't return proper response
                    const char *error_response =
                        "HTTP/1.1 500 Internal Server Error\r\n"
                        "Content-Type: text/plain\r\n"
                        "Content-Length: 21\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "Internal Server Error";

                    conn->response = strdup(error_response);
                    conn->response_len = strlen(error_response);
                }
            }
            else
            {
                // No handler function assigned
                const char *error_response =
                    "HTTP/1.1 501 Not Implemented\r\n"
                    "Content-Type: application/json\r\n"
                    "Content-Length: 42\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "{\"error\": \"API handler not implemented\"}";

                conn->response = strdup(error_response);
                conn->response_len = strlen(error_response);
                write_log("ERROR", "API handler function is NULL");
            }
            break;
        }

        case ROUTE_TYPE_STATIC:
        {
            // Handle static file serving (your original logic)
            char *root = "public";
            char full_path[2048];
            snprintf(full_path, sizeof(full_path), "%s%s", root, matched_route->handler.static_path);

            FILE *fp = fopen(full_path, "rb");
            if (!fp)
            {
                const char *not_found =
                    "HTTP/1.1 404 Not Found\r\n"
                    "Content-Type: text/plain\r\n"
                    "Content-Length: 17\r\n"
                    "Connection: close\r\n"
                    "\r\n"
                    "404 File Not Found";

                conn->response = strdup(not_found);
                conn->response_len = strlen(not_found);

                char error_log[2200];
                snprintf(error_log, sizeof(error_log), "Static file not found: %s", full_path);
                write_log("ERROR", error_log);
                break;
            }

            // Get file size
            fseek(fp, 0, SEEK_END);
            long file_size = ftell(fp);
            fseek(fp, 0, SEEK_SET);

            const char *mime = get_mime_type(matched_route->handler.static_path);
            char header[800];
            snprintf(header, sizeof(header),
                     "HTTP/1.1 200 OK\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %ld\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     mime, file_size);

            size_t header_len = strlen(header);
            conn->response = malloc(header_len + file_size + 1);
            if (conn->response)
            {
                memcpy(conn->response, header, header_len);
                size_t bytes_read = fread(conn->response + header_len, 1, file_size, fp);
                conn->response_len = header_len + bytes_read;
                conn->response[conn->response_len] = '\0';
            }
            fclose(fp);

            char static_log[400];
            snprintf(static_log, sizeof(static_log), "Served static file: %s", matched_route->handler.static_path);
            write_log("INFO", static_log);
            break;
        }

        case ROUTE_TYPE_REDIRECT:
        {
            // Handle redirect routes (your original logic)
            char redirect_response[800];
            snprintf(redirect_response, sizeof(redirect_response),
                     "HTTP/1.1 301 Moved Permanently\r\n"
                     "Location: %s\r\n"
                     "Content-Length: 0\r\n"
                     "Connection: close\r\n"
                     "\r\n",
                     matched_route->handler.redirect_url);

            conn->response = strdup(redirect_response);
            conn->response_len = strlen(redirect_response);

            char redirect_log[1400];
            snprintf(redirect_log, sizeof(redirect_log), "Redirected %s -> %s",
                     conn->parsed_request->path, matched_route->handler.redirect_url);
            write_log("INFO", redirect_log);
            break;
        }

        default:
        {
            const char *error_response =
                "HTTP/1.1 500 Internal Server Error\r\n"
                "Content-Type: application/json\r\n"
                "Content-Length: 32\r\n"
                "Connection: close\r\n"
                "\r\n"
                "{\"error\": \"Unknown route type\"}";

            conn->response = strdup(error_response);
            conn->response_len = strlen(error_response);
            write_log("ERROR", "Unknown route type encountered");
            break;
        }
        }
    }
    else
    {
        // No route matched - 404
        char not_found[1200];
        snprintf(not_found, sizeof(not_found),
                 "HTTP/1.1 404 Not Found\r\n"
                 "Content-Type: application/json\r\n"
                 "Content-Length: %zu\r\n"
                 "Connection: close\r\n"
                 "\r\n"
                 "{\"error\": \"Route not found\", \"path\": \"%s\"}",
                 strlen("{\"error\": \"Route not found\", \"path\": \"") + strlen(conn->parsed_request->path) + strlen("\"}"),
                 conn->parsed_request->path);

        conn->response = strdup(not_found);
        conn->response_len = strlen(not_found);

        char not_found_log[1300];
        snprintf(not_found_log, sizeof(not_found_log), "No route matched for path: %s", conn->parsed_request->path);
        write_log("ERROR", not_found_log);
    }

    conn->state = CONN_WRITING;
    return 0;
}
