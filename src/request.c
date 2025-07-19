#include "request.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int parse_http_request(const char *raw, HttpRequest *req) {
    memset(req, 0, sizeof(HttpRequest));

    const char *line_end = strstr(raw, "\r\n");
    if (!line_end) return -1;

    // --- Parse Request Line ---
    char request_line[1024];
    int len = line_end - raw;
    strncpy(request_line, raw, len);
    request_line[len] = '\0';

    sscanf(request_line, "%s %s", req->method, req->path);

    // --- Split query string ---
    char *qmark = strchr(req->path, '?');
    if (qmark) {
        *qmark = '\0';
        strcpy(req->query, qmark + 1);
        req->query_count = parse_query_params(req->query, req->query_params);
    }

    // --- Parse Headers ---
    const char *headers_start = line_end + 2;
    const char *headers_end = strstr(headers_start, "\r\n\r\n");
    if (!headers_end) return -1;

    char header_block[2048];
    len = headers_end - headers_start;
    strncpy(header_block, headers_start, len);
    header_block[len] = '\0';

    char *line = strtok(header_block, "\r\n");
    while (line && req->header_count < MAX_HEADERS) {
        char *colon = strchr(line, ':');
        if (colon) {
            *colon = '\0';
            strcpy(req->headers[req->header_count].key, line);
            strcpy(req->headers[req->header_count].value, colon + 2); // skip ": "
            req->header_count++;
        }
        line = strtok(NULL, "\r\n");
    }

    // --- Body ---
    req->body = headers_end + 4;

    return 0;
}

const char* get_header_value(const HttpRequest *req, const char *key) {
    for (int i = 0; i < req->header_count; i++) {
        if (strcasecmp(req->headers[i].key, key) == 0) {
            return req->headers[i].value;
        }
    }
    return NULL;
}
