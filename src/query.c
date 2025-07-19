#include "query.h"
#include <string.h>
#include <stdio.h>

int parse_query_params(const char *query_string, QueryParam *params) {
    int param_count = 0;
    char query_copy[1024];
    strncpy(query_copy, query_string, sizeof(query_copy));
    query_copy[sizeof(query_copy) - 1] = '\0';

    char *token = strtok(query_copy, "&");
    while (token != NULL && param_count < MAX_PARAMS) {
        sscanf(token, "%255[^=]=%255s", params[param_count].key, params[param_count].value);
        param_count++;
        token = strtok(NULL, "&");
    }
    return param_count;
}

const char *get_query_value(QueryParam *params, int count, const char *key) {
    for (int i = 0; i < count; i++) {
        if (strcmp(params[i].key, key) == 0) {
            return params[i].value;
        }
    }
    return NULL;
}
