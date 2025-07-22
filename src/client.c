#include "client.h"
#include "log.h"
#include "mime.h"
#include "query.h"
#include "router.h"
#include "request.h"
#include "api_handlers.h"
#include "globals.h"
#include "middleware.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 8192

void *handle_client(void *arg)
{
    increment_connections();

    int client_socket = *(int *)arg;
    free(arg);

    // Check if server is shutting down
    if (server_shutdown)
    {
        close(client_socket);
        decrement_connections();
        return NULL;
    }

    char buffer[BUFFER_SIZE] = {0};

    // Log client IP address
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    char log_msg[200]; // Increased buffer size
    snprintf(log_msg, sizeof(log_msg), "Connection from IP: %s", client_ip);
    write_log("INFO", log_msg);

    // Read request
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0';
        write_log("INFO", buffer);

        // Parse HTTP request
        HttpRequest req;
        if (parse_http_request(buffer, &req) != 0)
        {
            const char *err = "HTTP/1.1 400 Bad Request\r\n\r\nMalformed request";
            write(client_socket, err, strlen(err));
            write_log("ERROR", "Malformed HTTP request");
            close(client_socket);
            decrement_connections(); // decrement connection
            return NULL;
        }

        // Log parsed request details
        char method_log[1200]; // Increased buffer size
        snprintf(method_log, sizeof(method_log), "Method: %s, Path: %s", req.method, req.path);
        write_log("INFO", method_log);

        // Log query parameters
        for (int i = 0; i < req.query_count; i++)
        {
            char *log_entry;
            if (asprintf(&log_entry, "Query Param - %s: %s",
                         req.query_params[i].key, req.query_params[i].value) != -1)
            {
                write_log("INFO", log_entry);
                free(log_entry);
            }
            else
            {
                write_log("ERROR", "Failed to allocate memory for log_entry");
            }
        }

        // Log headers
        for (int i = 0; i < req.header_count; i++)
        {
            char header_log[600]; // Increased buffer size
            snprintf(header_log, sizeof(header_log), "Header - %s: %s",
                     req.headers[i].key, req.headers[i].value);
            write_log("INFO", header_log);
        }

        // Check method
        if (strcmp(req.method, "GET") != 0 &&
            strcmp(req.method, "POST") != 0 &&
            strcmp(req.method, "PUT") != 0 &&
            strcmp(req.method, "DELETE") != 0

        )
        {
            const char *err = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            write(client_socket, err, strlen(err));
            write_log("ERROR", "Method not allowed");
            close(client_socket);
            decrement_connections(); // decrement connection
            return NULL;
        }

        if (has_request_body(&req))
        {
            char body_log[300];
            snprintf(body_log, sizeof(body_log),
                     "Request body - Length: %zu, Type: %s",
                     get_content_length(&req),
                     get_content_type(&req));
            write_log("INFO", body_log);

            if (req.body)
            {
                char body_preview[100];
                snprintf(body_preview, sizeof(body_preview),
                         "Body preview: %.80s%s",
                         req.body,
                         strlen(req.body) > 80 ? "..." : "");
                write_log("INFO", body_preview);
            }
        }

        // UNIFIED ROUTE HANDLING
        RouteParam route_params[MAX_ROUTE_PARAMS];
        Route *matched_route = NULL;
        int param_count = check_route_with_method(req.path, req.method, route_params, &matched_route);
        if (param_count >= 0 && matched_route)
        {
            // Log the matched route
            char route_log[512]; // Increased buffer size
            snprintf(route_log, sizeof(route_log), "Matched route: %s (type: %d)",
                     matched_route->pattern, matched_route->type);
            write_log("INFO", route_log);

            switch (matched_route->type)
            {
            case ROUTE_TYPE_API:
            {
                // Log route parameters
                printf("API route matched with %d parameters:\n", param_count);
                for (int i = 0; i < param_count; i++)
                {
                    printf("  %s: %s\n", route_params[i].key, route_params[i].value);
                    char param_log[400]; // Increased buffer size
                    snprintf(param_log, sizeof(param_log), "Route Param - %s: %s",
                             route_params[i].key, route_params[i].value);
                    write_log("INFO", param_log);
                }
                


                  printf("API route matched with %d parameters:\n", param_count);
    
    // ADD THIS DEBUG SECTION
    printf("DEBUG: Route pointer: %p\n", (void*)matched_route);
    printf("DEBUG: Route pattern: %s\n", matched_route->pattern);
    printf("DEBUG: Route handler: %p\n", (void*)matched_route->handler.api_handler);
    printf("DEBUG: Route middleware: %p\n", (void*)matched_route->middleware);
                // DIRECT FUNCTION CALL
                if (matched_route->handler.api_handler)
                {
                    // Now dont call directly to handler first run middleware
                    ApiResponse api_response = {0};

                    // NEW: Execute middleware chain if present
                    if (matched_route->middleware)
                    {
                        char middleware_log[300];
                        snprintf(middleware_log, sizeof(middleware_log),
                                 "Executing middleware chain for route: %s", matched_route->pattern);
                        write_log("INFO", middleware_log);

                        middleware_result_t middleware_result = execute_middleware_chain(
                            matched_route->middleware, &req, &api_response);

                        // If middleware stopped execution, send the response and break
                        if (middleware_result == MIDDLEWARE_STOP)
                        {
                            write_log("INFO", "Middleware stopped execution - sending middleware response");

                            // Build and send HTTP response from middleware
                            char response_header[800];
                            snprintf(response_header, sizeof(response_header),
                                     "HTTP/1.1 %d %s\r\n"
                                     "Content-Type: %s\r\n"
                                     "Content-Length: %zu\r\n"
                                     "Access-Control-Allow-Origin: *\r\n"
                                     "\r\n",
                                     api_response.status_code,
                                     api_response.status_code == 200 ? "OK" : api_response.status_code == 401 ? "Unauthorized"
                                                                          : api_response.status_code == 400   ? "Bad Request"
                                                                          : api_response.status_code == 404   ? "Not Found"
                                                                                                              : "Error",
                                     api_response.content_type ? api_response.content_type : "text/plain",
                                     api_response.content ? strlen(api_response.content) : 0);

                            write(client_socket, response_header, strlen(response_header));
                            if (api_response.content)
                            {
                                write(client_socket, api_response.content, strlen(api_response.content));
                            }

                            // Clean up middleware response
                            free_api_response(&api_response);
                            break; // Exit the switch case
                        }

                        if (middleware_result == MIDDLEWARE_ERROR)
                        {
                            write_log("ERROR", "Middleware execution error");
                            api_response.status_code = 500;
                            if (api_response.content)
                                free(api_response.content);
                            if (api_response.content_type)
                                free(api_response.content_type);
                            api_response.content = strdup("Internal Server Error");
                            api_response.content_type = strdup("text/plain");

                            // Send error response
                            char response_header[800];
                            snprintf(response_header, sizeof(response_header),
                                     "HTTP/1.1 500 Internal Server Error\r\n"
                                     "Content-Type: text/plain\r\n"
                                     "Content-Length: %zu\r\n"
                                     "Access-Control-Allow-Origin: *\r\n"
                                     "\r\n",
                                     strlen(api_response.content));

                            write(client_socket, response_header, strlen(response_header));
                            write(client_socket, api_response.content, strlen(api_response.content));
                            free_api_response(&api_response);
                            break; // Exit the switch case
                        }

                        write_log("INFO", "Middleware chain executed successfully - continuing to route handler");
                    }

                    // Call the handler function directly (existing code)
                    api_response = matched_route->handler.api_handler(&req, route_params, param_count);

                    // Build and send HTTP response
                    char response_header[800]; // Increased buffer size
                    snprintf(response_header, sizeof(response_header),
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Type: %s\r\n"
                             "Content-Length: %zu\r\n"

                             "Access-Control-Allow-Origin: *\r\n"
                             "\r\n",
                             api_response.status_code,
                             api_response.status_code == 200 ? "OK" : api_response.status_code == 400 ? "Bad Request"
                                                                  : api_response.status_code == 404   ? "Not Found"
                                                                                                      : "Error",
                             api_response.content_type,
                             strlen(api_response.content));

                    write(client_socket, response_header, strlen(response_header));
                    write(client_socket, api_response.content, strlen(api_response.content));

                    // Log success
                    char success_log[300]; // Increased buffer size
                    snprintf(success_log, sizeof(success_log),
                             "API handler executed successfully, status: %d",
                             api_response.status_code);
                    write_log("INFO", success_log);

                    // Clean up response memory
                    free_api_response(&api_response);
                }
                else
                {
                    // No handler function assigned
                    const char *error_response =
                        "HTTP/1.1 501 Not Implemented\r\n"
                        "Content-Type: application/json\r\n"
                        "\r\n"
                        "{\"error\": \"API handler not implemented\"}";
                    write(client_socket, error_response, strlen(error_response));
                    write_log("ERROR", "API handler function is NULL");
                }
                break;
            }

            case ROUTE_TYPE_STATIC:
            {
                // Handle static file serving
                char *root = "public";
                char full_path[2048];
                snprintf(full_path, sizeof(full_path), "%s%s", root, matched_route->handler.static_path);

                FILE *fp = fopen(full_path, "rb");
                if (!fp)
                {
                    const char *not_found =
                        "HTTP/1.1 404 Not Found\r\n"
                        "Content-Type: text/plain\r\n"
                        "\r\n"
                        "404 File Not Found";
                    write(client_socket, not_found, strlen(not_found));

                    char error_log[2200]; // Increased buffer size
                    snprintf(error_log, sizeof(error_log), "Static file not found: %s", full_path);
                    write_log("ERROR", error_log);
                    break;
                }

                // Get file size for Content-Length header
                fseek(fp, 0, SEEK_END);
                long file_size = ftell(fp);
                fseek(fp, 0, SEEK_SET);

                const char *mime = get_mime_type(matched_route->handler.static_path);
                char header[800]; // Increased buffer size
                snprintf(header, sizeof(header),
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: %s\r\n"
                         "Content-Length: %ld\r\n"
                         "\r\n",
                         mime, file_size);

                write(client_socket, header, strlen(header));

                // Send file content in chunks
                char file_buf[1024];
                size_t bytes_read_file;
                while ((bytes_read_file = fread(file_buf, 1, sizeof(file_buf), fp)) > 0)
                {
                    write(client_socket, file_buf, bytes_read_file);
                }
                fclose(fp);

                char static_log[400]; // Increased buffer size
                snprintf(static_log, sizeof(static_log), "Served static file: %s", matched_route->handler.static_path);
                write_log("INFO", static_log);
                break;
            }

            case ROUTE_TYPE_REDIRECT:
            {
                // Handle redirect routes
                char redirect_response[800]; // Increased buffer size
                snprintf(redirect_response, sizeof(redirect_response),
                         "HTTP/1.1 301 Moved Permanently\r\n"
                         "Location: %s\r\n"
                         "Content-Length: 0\r\n"
                         "\r\n",
                         matched_route->handler.redirect_url);

                write(client_socket, redirect_response, strlen(redirect_response));

                char redirect_log[1400]; // Increased buffer size
                snprintf(redirect_log, sizeof(redirect_log), "Redirected %s -> %s",
                         req.path, matched_route->handler.redirect_url);
                write_log("INFO", redirect_log);
                break;
            }

            default:
            {
                // Unknown route type
                const char *error_response =
                    "HTTP/1.1 500 Internal Server Error\r\n"
                    "Content-Type: application/json\r\n"
                    "\r\n"
                    "{\"error\": \"Unknown route type\"}";
                write(client_socket, error_response, strlen(error_response));
                write_log("ERROR", "Unknown route type encountered");
                break;
            }
            }
        }
        else
        {
            // No route matched - 404
            char not_found[1200]; // Increased buffer size
            snprintf(not_found, sizeof(not_found),
                     "HTTP/1.1 404 Not Found\r\n"
                     "Content-Type: application/json\r\n"
                     "\r\n"
                     "{\"error\": \"Route not found\", \"path\": \"%s\"}",
                     req.path);
            write(client_socket, not_found, strlen(not_found));

            char not_found_log[1300]; // Increased buffer size
            snprintf(not_found_log, sizeof(not_found_log), "No route matched for path: %s", req.path);
            write_log("ERROR", not_found_log);
        }
        free_request(&req);
    }
    else
    {
        write_log("ERROR", "Failed to read from client");
    }

    close(client_socket);

    decrement_connections(); // decrement connection
    return NULL;
}
