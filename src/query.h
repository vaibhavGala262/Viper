#include <stdbool.h>
#include <stddef.h>
#ifndef QUERY_H
#define QUERY_H


#define MAX_PARAMS 20

typedef struct {
    char key[256];
    char value[256];
} QueryParam;

// Core query parameter parsing
int parse_query_params(const char *query_string, QueryParam *params);
int parse_query_string(const char *query_string, QueryParam *params, int max_params);
const char *get_query_value(QueryParam *params, int count, const char *key);

// URL decoding utilities (for query parameters and form data)
int url_decode(const char *src, char *dest, size_t dest_size);
int url_encode(const char *src, char *dest, size_t dest_size);

// Form data parsing (application/x-www-form-urlencoded)
typedef struct {
    char key[64];
    char value[256];
} FormParam;

int parse_form_data_string(const char *form_data, FormParam *params, int max_params);

// JSON parsing utilities (simple key-value extraction)
int extract_json_string(const char *json, const char *key, char *value, size_t value_size);
int extract_json_number(const char *json, const char *key, long *value);

// Query parameter utilities
bool has_query_param(QueryParam *params, int count, const char *key);
int get_query_param_count(const char *query_string);

#endif
