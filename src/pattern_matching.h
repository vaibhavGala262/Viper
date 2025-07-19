#ifndef PATTERN_MATCHING_H
#define PATTERN_MATCHING_H

#define MAX_ROUTE_PARAMS 16
#define PARAM_KEY_SIZE 64
#define PARAM_VALUE_SIZE 256

typedef struct {
    char key[PARAM_KEY_SIZE];     // Parameter name (without ':')
    char value[PARAM_VALUE_SIZE]; // Parameter value
} RouteParam;    // struct to store all params in the path 


int match_route(const char *pattern, const char *path, RouteParam *params, int max_params);

const char* get_route_param(const RouteParam *params, int param_count, const char *key);   // get as specific value corresponding to key 

/**
 * Print all route parameters (for debugging)
 * @param params Array of route parameters
 * @param param_count Number of parameters
 */
void print_route_params(const RouteParam *params, int param_count);  

#endif // PATTERN_MATCHING_H