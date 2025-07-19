#include "client.h"
#include "log.h"
#include "mime.h"
#include "query.h"
#include "router.h"

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

        char method[8], url[1024];
        sscanf(buffer, "%s %s", method, url); // Full request ex " GET /index.html HTTP/1.1\r\n..."
        // Fetches First two string sepparated by space in this case method = "GET" , path= "/index.html"

        char path[1024], query[1024] = "";

        char *qmark = strchr(url, '?'); // find '?'
        if (qmark)
        {
            *qmark = '\0';            // terminate path at '?'
            strcpy(path, url);        // store actual path
            strcpy(query, qmark + 1); // store query string
        }
        else
        {
            strcpy(path, url); // no query, just path
        }
        QueryParam params[MAX_PARAMS];
        int query_count = parse_query_params(query, params);
        for (int i = 0; i < query_count; i++)
        {
            char *log_entry;
            if (asprintf(&log_entry, "Query Param - %s: %s", params[i].key, params[i].value) != -1)
            {
                write_log("INFO", log_entry);
                free(log_entry);
            }
            else
            {
                write_log("ERROR", "Failed to allocate memory for log_entry");
            }
        }

        if (strcmp(method, "GET") != 0)
        {
            const char *err = "HTTP/1.1 405 Method Not Allowed\r\n\r\n";
            write(client_socket, err, strlen(err));
            close(client_socket);
            return NULL;
        } // Check if Method type is get tho we can define more types furthur

        resolve_path(path); // resolve path according to router.c

        char user_id[64]={0};
       
        if (match_route("/users/:id", path, user_id)) {
        printf("User ID: %s\n", user_id);
    }else{
        printf("No Match! \n");
    }


        char *root = "public"; // Static folder
        char full_path[2048];
        snprintf(full_path, sizeof(full_path), "%s%s", root, path);

        FILE *fp = fopen(full_path, "rb");
        if (!fp)
        {
            // File not found: send 404 response
            const char *not_found = "HTTP/1.1 404 Not Found\r\nContent-Type: text/plain\r\n\r\n404 Not Found";
            write(client_socket, not_found, strlen(not_found));
            write_log("ERROR", "File not found");
            close(client_socket);
            return NULL;
        }

        const char *mime = get_mime_type(path);
        char header[256];
        snprintf(header, sizeof(header),
                 "HTTP/1.1 200 OK\r\n"
                 "Content-Type: %s\r\n"
                 "\r\n",
                 mime);

        write(client_socket, header, strlen(header));

        char file_buf[1024];
        size_t bytes_read;

        while ((bytes_read = fread(file_buf, 1, sizeof(file_buf), fp)) > 0)
        {
            write(client_socket, file_buf, bytes_read);
        }
        fclose(fp);

        // Send response
    }
    else
    {
        write_log("ERROR", "Failed to read from client");
    }

    close(client_socket);
    return NULL;
}
