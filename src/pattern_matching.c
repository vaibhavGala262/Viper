#include "pattern_matching.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int match_route(const char *pattern, const char *path, RouteParam *params, int max_params) {
    if (!pattern || !path || !params) return -1;
    
    printf("Matching pattern: %s against path: %s\n", pattern, path);
    
    int param_count = 0;
    const char *pat_ptr = pattern;
    const char *path_ptr = path;
    
    // Skip leading slashes
    if (*pat_ptr == '/') pat_ptr++;
    if (*path_ptr == '/') path_ptr++;
    
    while (*pat_ptr && *path_ptr && param_count < max_params) {
        // Find the end of current segment in pattern
        const char *pat_end = strchr(pat_ptr, '/');
        if (!pat_end) pat_end = pat_ptr + strlen(pat_ptr);
        
        // Find the end of current segment in path
        const char *path_end = strchr(path_ptr, '/');
        if (!path_end) path_end = path_ptr + strlen(path_ptr);
        
        // Extract segments
        int pat_len = pat_end - pat_ptr;
        int path_len = path_end - path_ptr;
        
        if (pat_len >= PARAM_KEY_SIZE || path_len >= PARAM_VALUE_SIZE) {
            return -1; // Segment too long
        }
        
        char pat_segment[PARAM_KEY_SIZE];
        char path_segment[PARAM_VALUE_SIZE];
        
        strncpy(pat_segment, pat_ptr, pat_len);
        pat_segment[pat_len] = '\0';
        strncpy(path_segment, path_ptr, path_len);
        path_segment[path_len] = '\0';
        
        // Check if pattern segment is a parameter
        if (pat_segment[0] == ':') {
            // Extract parameter name (skip ':')
            strncpy(params[param_count].key, pat_segment + 1, PARAM_KEY_SIZE - 1);
            params[param_count].key[PARAM_KEY_SIZE - 1] = '\0';
            
            // Extract parameter value
            strncpy(params[param_count].value, path_segment, PARAM_VALUE_SIZE - 1);
            params[param_count].value[PARAM_VALUE_SIZE - 1] = '\0';
            
            param_count++;
        } else if (strcmp(pat_segment, path_segment) != 0) {
            // Static segments must match exactly
            return -1;
        }
        
        // Move to next segments
        pat_ptr = (*pat_end == '/') ? pat_end + 1 : pat_end;
        path_ptr = (*path_end == '/') ? path_end + 1 : path_end;
    }
    
    // Both should be at end for complete match
    if (*pat_ptr || *path_ptr) {
        return -1;
    }
    
    if (param_count > 0) {
        printf("Extracted %d parameters\n", param_count);
    }
    
    return param_count;
}

const char* get_route_param(const RouteParam *params, int param_count, const char *key) {
    if (!params || !key) return NULL;
    
    for (int i = 0; i < param_count; i++) {
        if (strcmp(params[i].key, key) == 0) {
            return params[i].value;
        }
    }
    return NULL;
}

void print_route_params(const RouteParam *params, int param_count) {
    printf("Route Parameters (%d):\n", param_count);
    for (int i = 0; i < param_count; i++) {
        printf("  %s: %s\n", params[i].key, params[i].value);
    }
}
