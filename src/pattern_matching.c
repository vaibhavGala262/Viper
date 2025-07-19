#include <stdio.h>
#include "pattern_matching.h"
#include <string.h>

int match_route(const char *pattern, const char *path, char *param_out) {
    printf("Requested path: %s\n", path);
    
    // Clear param_out initially
    param_out[0] = '\0';
    
    const char *pat_ptr = pattern;
    const char *path_ptr = path;
    
    // Skip leading slashes
    if (*pat_ptr == '/') pat_ptr++;
    if (*path_ptr == '/') path_ptr++;
    
    while (*pat_ptr && *path_ptr) {
        // Find the end of current segment in pattern
        const char *pat_end = strchr(pat_ptr, '/');
        if (!pat_end) pat_end = pat_ptr + strlen(pat_ptr);
        
        // Find the end of current segment in path
        const char *path_end = strchr(path_ptr, '/');
        if (!path_end) path_end = path_ptr + strlen(path_ptr);
        
        // Extract segments
        int pat_len = pat_end - pat_ptr;
        int path_len = path_end - path_ptr;
        
        char pat_segment[256], path_segment[256];
        strncpy(pat_segment, pat_ptr, pat_len);
        pat_segment[pat_len] = '\0';
        strncpy(path_segment, path_ptr, path_len);
        path_segment[path_len] = '\0';
        
        // Check if pattern segment is a parameter
        if (pat_segment[0] == ':') {
            // Extract parameter value
            strcpy(param_out, path_segment);
        } else if (strcmp(pat_segment, path_segment) != 0) {
            // Static segments must match exactly
            return 0;
        }
        
        // Move to next segments
        pat_ptr = (*pat_end == '/') ? pat_end + 1 : pat_end;
        path_ptr = (*path_end == '/') ? path_end + 1 : path_end;
    }
    
    // Both should be at end for complete match
    if (*pat_ptr || *path_ptr) {
        return 0;
    }
    
    if (param_out[0] != '\0') {
        printf("Matched param: %s\n", param_out);
    }
    
    return 1;  // matched
}
