#ifndef QUERY_H
#define QUERY_H


#define MAX_PARAMS 20

typedef struct {
    char key[256];
    char value[256];
}QueryParam;

int parse_query_params(const char *query_string, QueryParam *params);
const char *get_query_value(QueryParam *params, int count, const char *key);

#endif

