#include "request.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


int parse_http_request(const char *raw, HttpRequest *req) {
    if (!raw || !req) return -1;
    
    printf("DEBUG: Raw request length: %zu\n", strlen(raw));
    printf("DEBUG: Raw request:\n%s\n", raw);
    
    // Initialize request structure
    memset(req, 0, sizeof(HttpRequest));
    req->body = NULL;
    req->body_length = 0;
    req->content_length = 0;
    req->has_body = false;
    
    // Find end of headers (double CRLF)
    const char *headers_end = strstr(raw, "\r\n\r\n");
    int headers_end_length = 4;
    
    if (!headers_end) {
        headers_end = strstr(raw, "\n\n"); // Handle single LF
        if (!headers_end) {
            printf("DEBUG: No header end found\n");
            return -1;
        }
        headers_end_length = 2;
    }
    
    size_t headers_length = headers_end - raw;
    printf("DEBUG: Headers length: %zu\n", headers_length);
    
    // Parse request line and headers
    char *headers_copy = malloc(headers_length + 1);
    if (!headers_copy) return -1;
    
    strncpy(headers_copy, raw, headers_length);
    headers_copy[headers_length] = '\0';
    
    printf("DEBUG: Headers section:\n%s\n", headers_copy);
    
    // Parse first line (Method, Path, Version)
    char *save_ptr;
    char *line = strtok_r(headers_copy, "\r\n", &save_ptr);
    if (!line) {
        printf("DEBUG: No first line found\n");
        free(headers_copy);
        return -1;
    }
    
    printf("DEBUG: First line: %s\n", line);
    
    // Parse the request line: METHOD PATH VERSION
    char *method = strtok(line, " ");
    char *full_path = strtok(NULL, " ");
    char *version = strtok(NULL, " ");  // ADD HTTP version extraction
    
    if (!method || !full_path || !version) {
        printf("DEBUG: Invalid first line format - missing method, path, or version\n");
        free(headers_copy);
        return -1;
    }
    
    printf("DEBUG: Method: '%s', Path: '%s', Version: '%s'\n", method, full_path, version);
    
    // Copy method with null termination
    strncpy(req->method, method, sizeof(req->method) - 1);
    req->method[sizeof(req->method) - 1] = '\0';
    
    // Copy HTTP version with null termination
    strncpy(req->version, version, sizeof(req->version) - 1);
    req->version[sizeof(req->version) - 1] = '\0';
   
    // Parse path and query
    char *query_start = strchr(full_path, '?');
    if (query_start) {
        *query_start = '\0';
        strncpy(req->query, query_start + 1, sizeof(req->query) - 1);
        req->query[sizeof(req->query) - 1] = '\0';
        req->query_count = parse_query_string(req->query, req->query_params, MAX_PARAMS);
    }
    strncpy(req->path, full_path, sizeof(req->path) - 1);
    req->path[sizeof(req->path) - 1] = '\0';
    
    // Parse headers using strtok_r for thread safety
    req->header_count = 0;
    while ((line = strtok_r(NULL, "\r\n", &save_ptr)) && req->header_count < MAX_HEADERS) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            char *key = line;
            char *value = colon + 1;
            
            // Trim whitespace from value
            while (*value == ' ' || *value == '\t') value++;
            
            // Copy with null termination
            strncpy(req->headers[req->header_count].key, key, MAX_HEADER_KEY - 1);
            req->headers[req->header_count].key[MAX_HEADER_KEY - 1] = '\0';
            strncpy(req->headers[req->header_count].value, value, MAX_HEADER_VALUE - 1);
            req->headers[req->header_count].value[MAX_HEADER_VALUE - 1] = '\0';
            
            printf("DEBUG: Header: '%s' = '%s'\n", key, value);
            
            req->header_count++;
        }
    }
    
    free(headers_copy);
    
    printf("DEBUG: Parsed %d headers\n", req->header_count);
    
    // Parse Content-Length and Content-Type headers
    const char *content_length_str = get_header_value(req, "Content-Length");
    const char *content_type_str = get_header_value(req, "Content-Type");
    
    printf("DEBUG: Content-Length header: %s\n", content_length_str ? content_length_str : "NULL");
    printf("DEBUG: Content-Type header: %s\n", content_type_str ? content_type_str : "NULL");
    
    if (content_length_str) {
        req->content_length = strtoul(content_length_str, NULL, 10);
        req->has_body = (req->content_length > 0);
        printf("DEBUG: Parsed content_length: %zu, has_body: %s\n", 
               req->content_length, req->has_body ? "true" : "false");
    }
    
    if (content_type_str) {
        strncpy(req->content_type, content_type_str, sizeof(req->content_type) - 1);
        req->content_type[sizeof(req->content_type) - 1] = '\0';
    }
    
    // Handle request body if present
    if (req->has_body && req->content_length > 0) {
        printf("DEBUG: Processing request body...\n");
        
        const char *body_start = strstr(raw, "\r\n\r\n");
        if (!body_start) {
            body_start = strstr(raw, "\n\n");
            if (body_start) {
                body_start += 2;
            }
        } else {
            body_start += 4;
        }
        
        if (body_start) {
            printf("DEBUG: Body starts at offset: %ld\n", body_start - raw);
            printf("DEBUG: Expected body length: %zu\n", req->content_length);
            
            size_t remaining_data = strlen(body_start);
            printf("DEBUG: Remaining data length: %zu\n", remaining_data);
            
            if (req->content_length <= MAX_BODY_SIZE && remaining_data >= req->content_length) {
                req->body = malloc(req->content_length + 1);
                if (req->body) {
                    memcpy(req->body, body_start, req->content_length);
                    req->body[req->content_length] = '\0';
                    req->body_length = req->content_length;
                    
                    printf("DEBUG: Body successfully parsed: '%s'\n", req->body);
                } else {
                    printf("DEBUG: Failed to allocate memory for body\n");
                }
            } else {
                printf("DEBUG: Body too large (%zu > %d) or insufficient data (%zu < %zu)\n", 
                       req->content_length, MAX_BODY_SIZE, remaining_data, req->content_length);
            }
        } else {
            printf("DEBUG: Could not find body start\n");
        }
    }
    
    printf("DEBUG: Final parsed request - Method: '%s', Path: '%s', Version: '%s', Content-Length: %zu, Has Body: %s\n",
           req->method, req->path, req->version, req->content_length, req->has_body ? "true" : "false");
    
    return 0;
}

const char* get_header_value(const HttpRequest *req, const char *key) {
    if (!req || !key) return NULL;
    
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}

size_t get_content_length(const HttpRequest *req) {
    return req ? req->content_length : 0;
}

const char* get_content_type(const HttpRequest *req) {
    return req ? req->content_type : NULL;
}

bool has_request_body(const HttpRequest *req) {
    return req ? req->has_body : false;
}

// Simple JSON value extraction
int parse_json_body(const HttpRequest *req, const char *key, char *value, size_t value_size) {
    if (!req || !req->body || !key || !value) return -1;
    
    // Simple JSON parsing - look for "key":"value" pattern
    char search_pattern[128];
    snprintf(search_pattern, sizeof(search_pattern), "\"%s\":", key);
    
    char *key_pos = strstr(req->body, search_pattern);
    if (!key_pos) return -1;
    
    char *value_start = strchr(key_pos + strlen(search_pattern), '"');
    if (!value_start) return -1;
    value_start++; // Skip opening quote
    
    char *value_end = strchr(value_start, '"');
    if (!value_end) return -1;
    
    size_t length = value_end - value_start;
    if (length >= value_size) length = value_size - 1;
    
    strncpy(value, value_start, length);
    value[length] = '\0';
    
    return 0;
}

// Form data parsing for application/x-www-form-urlencoded
int parse_form_data(const HttpRequest *req, FormParam *params, int max_params) {
    if (!req || !req->body || !params) return -1;
    
    // Check if it's form data
    if (!strstr(req->content_type, "application/x-www-form-urlencoded")) {
        return -1;
    }
    
    // Parse form data (similar to query string parsing)
    char *body_copy = strdup(req->body);
    if (!body_copy) return -1;
    
    int param_count = 0;
    char *pair = strtok(body_copy, "&");
    
    while (pair && param_count < max_params) {
        char *equals = strchr(pair, '=');
        if (equals) {
            *equals = '\0';
            char *key = pair;
            char *value = equals + 1;
            
            strncpy(params[param_count].key, key, sizeof(params[param_count].key) - 1);
            strncpy(params[param_count].value, value, sizeof(params[param_count].value) - 1);
            param_count++;
        }
        pair = strtok(NULL, "&");
    }
    
    free(body_copy);
    return param_count;
}

bool is_http_1_1(HttpRequest *req) {
    if (!req) return false;
    
    // Check if version is HTTP/1.1 (case insensitive)
    return strcasecmp(req->version, "HTTP/1.1") == 0;
}

bool should_keep_alive(HttpRequest *req) {
    if (!req) return false;
    
    const char *connection = get_header_value(req, "Connection");
    
    if (is_http_1_1(req)) {
        // HTTP/1.1: Keep-alive is DEFAULT unless "Connection: close"
        if (!connection) {
            return true;  // Default to keep-alive
        }
        return strcasecmp(connection, "close") != 0;  // Keep alive unless explicitly "close"
    } else {
        // HTTP/1.0: Connection closes by DEFAULT unless "Connection: keep-alive"
        if (!connection) {
            return false;  // Default to close connection
        }
        return strcasecmp(connection, "keep-alive") == 0;  // Keep alive only if explicitly requested
    }
}



void free_request(HttpRequest *req) {
    if (req && req->body) {
        free(req->body);
        req->body = NULL;
        req->body_length = 0;
    }
}
