#ifndef REQUEST_H
#define REQUEST_H

#include "query.h"
#include "pattern_matching.h"

#define MAX_HEADERS 32
#define MAX_HEADER_KEY 64
#define MAX_HEADER_VALUE 256

typedef struct {
    char method[8];
    char path[1024];
    char query[1024];
    QueryParam query_params[MAX_PARAMS];
    int query_count;

    struct {
        char key[MAX_HEADER_KEY];
        char value[MAX_HEADER_VALUE];
    } headers[MAX_HEADERS];
    int header_count;

    const char *body;
} HttpRequest;

int parse_http_request(const char *raw, HttpRequest *req);
const char* get_header_value(const HttpRequest *req, const char *key);

#endif
