#include "client.h"
#include "log.h"
#include "mime.h"
#include "query.h"
#include "router.h"
#include "request.h"  // Add this include

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
        if (parse_http_request(buffer, &req) != 0) {
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
        for (int i = 0; i < req.query_count; i++) {
            char *log_entry;
            if (asprintf(&log_entry, "Query Param - %s: %s", 
                        req.query_params[i].key, req.query_params[i].value) != -1) {
                write_log("INFO", log_entry);
                free(log_entry);
            } else {
                write_log("ERROR", "Failed to allocate memory for log_entry");
            }
        }

        // Log headers
        for (int i = 0; i < req.header_count; i++) {
            char header_log[256];
            snprintf(header_log, sizeof(header_log), "Header - %s: %s", 
                    req.headers[i].key, req.headers[i].value);
            write_log("INFO", header_log);
        }

        // Check method
        if (strcmp(req.method, "GET") != 0) {
            const char *err = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            write(client_socket, err, strlen(err));
            close(client_socket);
            return NULL;
        }

        // Check API routes
        RouteParam route_params[MAX_ROUTE_PARAMS];
        int param_count = check_api_route(req.path, route_params);

        if (param_count >= 0) {
            // API route matched!
            printf("API route matched with %d parameters:\n", param_count);

            // Print all parameters
            for (int i = 0; i < param_count; i++) {
                printf("  %s: %s\n", route_params[i].key, route_params[i].value);

                // Log each parameter
                char param_log[128];
                snprintf(param_log, sizeof(param_log), "Route Param - %s: %s",
                         route_params[i].key, route_params[i].value);
                write_log("INFO", param_log);
            }

            // Handle different API endpoints
            const char *user_id = get_route_param(route_params, param_count, "id");
            const char *post_id = get_route_param(route_params, param_count, "post_id");
            const char *category = get_route_param(route_params, param_count, "category");

            // You can also access headers if needed for API authentication, etc.
            const char *auth_header = get_header_value(&req, "Authorization");
            const char *content_type = get_header_value(&req, "Content-Type");
            
            if (auth_header) {
                write_log("INFO", "Authorization header found");
            }

            // If you reach here, send a generic API response
            const char *generic_api = "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n{\"message\": \"API endpoint matched\"}";
            write(client_socket, generic_api, strlen(generic_api));
            close(client_socket);
            return NULL;
        }

        // Static file serving
        resolve_path(req.path); // resolve path according to router.c

        char *root = "public"; // Static folder
        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "%s%s", root, req.path);

        FILE *fp = fopen(full_path, "rb");
        if (!fp) {
            // File not found: send 404 response
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
            write(client_socket, not_found, strlen(not_found));
            write_log("ERROR", "File not found");
            close(client_socket);
            return NULL;
        }

        const char *mime = get_mime_type(req.path);
        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "\r\n",
                 mime);

        write(client_socket, header, strlen(header));

        char file_buf[1024];
        size_t bytes_read_file;

        while ((bytes_read_file = fread(file_buf, 1, sizeof(file_buf), fp)) > 0) {
            write(client_socket, file_buf, bytes_read_file);
        }
        fclose(fp);
    }
    else {
        write_log("ERROR", "Failed to read from client");
    }

    close(client_socket);
    return NULL;
}
