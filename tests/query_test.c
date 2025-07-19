// tests/test_query.c


// compile usingg gcc query_test.c ../src/query.c -o query_test
#include "../src/query.h"
#include <stdio.h>
#include <string.h>

int main() {
    const char *query_string = "download=true&version=2&debug=on";

    QueryParam params[MAX_PARAMS];
    int count = parse_query_params(query_string, params);

    printf("Parsed %d query parameters:\n", count);
    for (int i = 0; i < count; i++) {
        printf("Key: %s, Value: %s\n", params[i].key, params[i].value);
    }

    const char *version = get_query_value(params, count, "version");
    if (version) {
        printf("Version is: %s\n", version);
    } else {
        printf("Version not found.\n");
    }

    const char *download = get_query_value(params, count, "download");
    if (download && strcmp(download, "true") == 0) {
        printf("Download enabled!\n");
    }

    return 0;
}
