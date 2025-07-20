#include "client.h"
#include "log.h"
#include "mime.h"
#include "query.h"
#include "router.h"
#include "request.h" 
#include "api_handlers.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

#define BUFFER_SIZE 1024

void *handle_client(void *arg)
{
    int client_socket = *(int *)arg;
    free(arg);

    char buffer[BUFFER_SIZE] = {0};

    // Log client IP address
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket, (struct sockaddr *)&client_addr, &addr_len);

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);

    char log_msg[128];
    snprintf(log_msg, sizeof(log_msg), "Connection from IP: %s", client_ip);
    write_log("INFO", log_msg);

    // Read request
    ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
    if (bytes_read > 0)
    {
        buffer[bytes_read] = '\0'; // Null-terminate
        write_log("INFO", buffer); // Log raw request

        // Parse HTTP request using the new request module
        HttpRequest req;
        if (parse_http_request(buffer, &req) != 0)
        {
            const char *err = "HTTP/1.1 400 Bad Request\r\n\r\nMalformed request";
            write(client_socket, err, strlen(err));
            close(client_socket);
            return NULL;
        }

        // Log parsed request details
        char method_log[128];
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
            char header_log[256];
            snprintf(header_log, sizeof(header_log), "Header - %s: %s",
                     req.headers[i].key, req.headers[i].value);
            write_log("INFO", header_log);
        }

        // Check method
        if (strcmp(req.method, "GET") != 0)
        {
            const char *err = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            write(client_socket, err, strlen(err));
            close(client_socket);
            return NULL;
        }

        // UNIFIED ROUTE HANDLING No Diff logic for static and dynamic requests:
        RouteParam route_params[MAX_ROUTE_PARAMS];
        Route *matched_route = NULL;
        int param_count = check_route(req.path, route_params, &matched_route);

        if (param_count >= 0 && matched_route)
        {
            // Log the matched route
            char route_log[256];
            snprintf(route_log, sizeof(route_log), "Matched route: %s -> %s (type: %d)",
                     matched_route->pattern, matched_route->handler_name, matched_route->type);
            write_log("INFO", route_log);

            switch (matched_route->type)
            {
            case ROUTE_TYPE_API:
            {
                // Find the actual handler function
                HandlerFunc handler = find_handler_function(matched_route->handler_name);

                if (handler)
                {
                    // Log parameters 
                    printf("API route matched with %d parameters:\n", param_count);
                    for (int i = 0; i < param_count; i++)
                    {
                        printf("  %s: %s\n", route_params[i].key, route_params[i].value);
                        char param_log[128];
                        snprintf(param_log, sizeof(param_log), "Route Param - %s: %s",
                                 route_params[i].key, route_params[i].value);
                        write_log("INFO", param_log);
                    }

                    // EXECUTE THE ACTUAL HANDLER FUNCTION
                    ApiResponse api_response = handler(&req, route_params, param_count);

                    // Send the real response
                    char response_header[512];
                    snprintf(response_header, sizeof(response_header),
                             "HTTP/1.1 %d %s\r\n"
                             "Content-Type: %s\r\n"
                             "Content-Length: %zu\r\n"
                             "\r\n",
                             api_response.status_code,
                             api_response.status_code == 200 ? "OK" : "Error",
                             api_response.content_type,
                             strlen(api_response.content));

                    write(client_socket, response_header, strlen(response_header));
                    write(client_socket, api_response.content, strlen(api_response.content));

                    // Clean up memory
                    free_api_response(&api_response);

                    char success_log[256];
                    snprintf(success_log, sizeof(success_log), "API handler '%s' executed successfully",
                             matched_route->handler_name);
                    write_log("INFO", success_log);
                }
                else
                {
                    // Handler function not found
                    const char *error_response = "HTTP/1.1 501 Not Implemented\r\nContent-Type: application/json\r\n\r\n{\"error\": \"Handler not implemented\"}";
                    write(client_socket, error_response, strlen(error_response));

                    char error_log[256];
                    snprintf(error_log, sizeof(error_log), "Handler function '%s' not found",
                             matched_route->handler_name);
                    write_log("ERROR", error_log);
                }
                break;
            }

            case ROUTE_TYPE_STATIC:
                // Handle static file serving
                char *root = "public";
                char full_path[2048];
                snprintf(full_path, sizeof(full_path), "%s%s", root, matched_route->handler_name);

                FILE *fp = fopen(full_path, "rb");
                if (!fp)
                {
                    const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 File Not Found";
                    write(client_socket, not_found, strlen(not_found));
                    write_log("ERROR", "Static file not found");
                    close(client_socket);
                    return NULL;
                }

                const char *mime = get_mime_type(matched_route->handler_name);
                char header[256];
                snprintf(header, sizeof(header),
                         "HTTP/1.1 200 OK\r\n"
                         "Content-Type: %s\r\n"
                         "\r\n",
                         mime);

                write(client_socket, header, strlen(header));

                char file_buf[1024];
                size_t bytes_read_file;
                while ((bytes_read_file = fread(file_buf, 1, sizeof(file_buf), fp)) > 0)
                {
                    write(client_socket, file_buf, bytes_read_file);
                }
                fclose(fp);
                break;

            case ROUTE_TYPE_REDIRECT:
                // Handle redirect routes
                char redirect_response[512];
                snprintf(redirect_response, sizeof(redirect_response),
                         "HTTP/1.1 301 Moved Permanently\r\n"
                         "Location: %s\r\n"
                         "Content-Length: 0\r\n"
                         "\r\n",
                         matched_route->handler_name);

                write(client_socket, redirect_response, strlen(redirect_response));

                char redirect_log[256];
                snprintf(redirect_log, sizeof(redirect_log), "Redirected %s -> %s",
                         req.path, matched_route->handler_name);
                write_log("INFO", redirect_log);
                break;
            }
        }
        else
        {
            // No route matched - 404
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Route Not Found";
            write(client_socket, not_found, strlen(not_found));

            char not_found_log[256];
            snprintf(not_found_log, sizeof(not_found_log), "No route matched for path: %s", req.path);
            write_log("ERROR", not_found_log);
        }
    }
    else
    {
        write_log("ERROR", "Failed to read from client");
    }

    close(client_socket);
    return NULL;
}
