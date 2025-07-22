#ifndef REQUEST_H
#define REQUEST_H

#include "query.h"
#include <stddef.h>
#include <stdbool.h>
#include "pattern_matching.h"
#include "query.h"          
#include "pattern_matching.h"

#define MAX_HEADERS 32
#define MAX_HEADER_KEY 64
#define MAX_HEADER_VALUE 256
#define MAX_BODY_SIZE 8192  // Maximum body size (8KB)

typedef struct {
    char method[16];
    char path[1024];
    char version[16];           //  stores "HTTP/1.1" or "HTTP/1.0" for keep alive
    char query[1024];
    QueryParam query_params[MAX_PARAMS];
    int query_count;

    struct {
        char key[MAX_HEADER_KEY];
        char value[MAX_HEADER_VALUE];
    } headers[MAX_HEADERS];
    int header_count;

    // Enhanced body handling
    char *body;
    size_t body_length;   // length of body 
    size_t content_length;
    char content_type[128];
    bool has_body;
} HttpRequest;


// Core parsing functions
int parse_http_request(const char *raw, HttpRequest *req);
const char* get_header_value(const HttpRequest *req, const char *key);

// Body handling functions
int parse_request_body(HttpRequest *req, const char *raw_request, size_t raw_length);
size_t get_content_length(const HttpRequest *req);
const char* get_content_type(const HttpRequest *req);
bool has_request_body(const HttpRequest *req);

// JSON body parsing (for API endpoints)
int parse_json_body(const HttpRequest *req, const char *key, char *value, size_t value_size);


// Form data parsing (for HTML forms)

int parse_form_data(const HttpRequest *req, FormParam *params, int max_params);

// Cleanup function (Free Mem)
void free_request(HttpRequest *req);

bool should_keep_alive(HttpRequest *req);   // to check if keep alive connection or not 
bool is_http_1_1(HttpRequest *req);


#endif
