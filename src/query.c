#include "query.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

// Your existing function - keep it for backward compatibility
int parse_query_params(const char *query_string, QueryParam *params) {
    return parse_query_string(query_string, params, MAX_PARAMS);
}

// Enhanced version with max_params parameter
int parse_query_string(const char *query_string, QueryParam *params, int max_params) {
    if (!query_string || !params) return 0;
    
    int count = 0;
    char *query_copy = strdup(query_string);
    if (!query_copy) return 0;
    
    char *pair = strtok(query_copy, "&");
    while (pair && count < max_params) {
        char *equals = strchr(pair, '=');
        if (equals) {
            *equals = '\0';
            char *key = pair;
            char *value = equals + 1;
            
            // URL decode key and value
            url_decode(key, params[count].key, sizeof(params[count].key));
            url_decode(value, params[count].value, sizeof(params[count].value));
            count++;
        }
        pair = strtok(NULL, "&");
    }
    
    free(query_copy);
    return count;
}

const char *get_query_value(QueryParam *params, int count, const char *key) {
    if (!params || !key) return NULL;
    
    for (int i = 0; i < count; i++) {
        if (strcmp(params[i].key, key) == 0) {
            return params[i].value;
        }
    }
    return NULL;
}

// URL decoding function
int url_decode(const char *src, char *dest, size_t dest_size) {
    if (!src || !dest || dest_size == 0) return -1;
    
    size_t src_len = strlen(src);
    size_t dest_idx = 0;
    
    for (size_t i = 0; i < src_len && dest_idx < dest_size - 1; i++) {
        if (src[i] == '%' && i + 2 < src_len) {
            // Decode %XX hex sequence
            char hex[3] = {src[i+1], src[i+2], '\0'};
            if (isxdigit(hex[0]) && isxdigit(hex[1])) {
                dest[dest_idx++] = (char)strtol(hex, NULL, 16);
                i += 2; // Skip the two hex digits
            } else {
                dest[dest_idx++] = src[i];
            }
        } else if (src[i] == '+') {
            // Convert + to space (common in form data)
            dest[dest_idx++] = ' ';
        } else {
            dest[dest_idx++] = src[i];
        }
    }
    
    dest[dest_idx] = '\0';
    return dest_idx;
}

// URL encoding function
int url_encode(const char *src, char *dest, size_t dest_size) {
    if (!src || !dest || dest_size == 0) return -1;
    
    size_t src_len = strlen(src);
    size_t dest_idx = 0;
    
    for (size_t i = 0; i < src_len && dest_idx < dest_size - 1; i++) {
        char c = src[i];
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            // Safe characters - no encoding needed
            dest[dest_idx++] = c;
        } else {
            // Encode special characters
            if (dest_idx < dest_size - 3) {
                sprintf(dest + dest_idx, "%%%02X", (unsigned char)c);
                dest_idx += 3;
            } else {
                break; // Not enough space
            }
        }
    }
    
    dest[dest_idx] = '\0';
    return dest_idx;
}

// Form data parsing (same format as query string but from POST body)
int parse_form_data_string(const char *form_data, FormParam *params, int max_params) {
    if (!form_data || !params) return 0;
    
    int count = 0;
    char *data_copy = strdup(form_data);
    if (!data_copy) return 0;
    
    char *pair = strtok(data_copy, "&");
    while (pair && count < max_params) {
        char *equals = strchr(pair, '=');
        if (equals) {
            *equals = '\0';
            char *key = pair;
            char *value = equals + 1;
            
            // URL decode key and value
            url_decode(key, params[count].key, sizeof(params[count].key));
            url_decode(value, params[count].value, sizeof(params[count].value));
            count++;
        }
        pair = strtok(NULL, "&");
    }
    
    free(data_copy);
    return count;
}

// Simple JSON string extraction
int extract_json_string(const char *json, const char *key, char *value, size_t value_size) {
    if (!json || !key || !value) return -1;
    
    // Look for "key":"value" pattern
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    
    char *key_pos = strstr(json, pattern);
    if (!key_pos) return -1;
    
    // Find the value after the colon
    char *colon = strchr(key_pos, ':');
    if (!colon) return -1;
    
    // Skip whitespace and find opening quote
    char *value_start = colon + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) {
        value_start++;
    }
    
    if (*value_start != '"') return -1; // Not a string value
    value_start++; // Skip opening quote
    
    // Find closing quote
    char *value_end = value_start;
    while (*value_end && *value_end != '"') {
        if (*value_end == '\\') {
            value_end++; // Skip escaped character
            if (*value_end) value_end++;
        } else {
            value_end++;
        }
    }
    
    if (*value_end != '"') return -1; // No closing quote found
    
    // Copy value
    size_t length = value_end - value_start;
    if (length >= value_size) length = value_size - 1;
    
    strncpy(value, value_start, length);
    value[length] = '\0';
    
    return 0;
}

// Simple JSON number extraction
int extract_json_number(const char *json, const char *key, long *value) {
    if (!json || !key || !value) return -1;
    
    char pattern[128];
    snprintf(pattern, sizeof(pattern), "\"%s\":", key);
    
    char *key_pos = strstr(json, pattern);
    if (!key_pos) return -1;
    
    char *colon = strchr(key_pos, ':');
    if (!colon) return -1;
    
    // Skip whitespace
    char *value_start = colon + 1;
    while (*value_start && (*value_start == ' ' || *value_start == '\t')) {
        value_start++;
    }
    
    // Parse number
    char *endptr;
    *value = strtol(value_start, &endptr, 10);
    
    if (endptr == value_start) return -1; // No number found
    
    return 0;
}

// Utility functions
bool has_query_param(QueryParam *params, int count, const char *key) {
    return get_query_value(params, count, key) != NULL;
}

int get_query_param_count(const char *query_string) {
    if (!query_string || strlen(query_string) == 0) return 0;
    
    int count = 1; // At least one parameter if string is not empty
    const char *p = query_string;
    
    while ((p = strchr(p, '&')) != NULL) {
        count++;
        p++; // Move past the '&'
    }
    
    return count;
}
