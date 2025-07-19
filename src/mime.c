#include <string.h>
#include "mime.h"

const char *get_mime_type(const char *path)
{
    if (strstr(path, ".html"))
        return "text/html";
    if (strstr(path, ".css"))
        return "text/css";
    if (strstr(path, ".js"))
        return "application/javascript";
    if (strstr(path, ".png"))
        return "image/png";
    if (strstr(path, ".jpg") || strstr(path, ".jpeg"))
        return "image/jpeg";
    if (strstr(path, ".svg"))
        return "image/svg+xml";
    if (strstr(path, ".ico"))
        return "image/x-icon";
    if (strstr(path, ".json"))
        return "application/json";
    if (strstr(path, ".pdf"))
        return "application/pdf";
    return "application/octet-stream";
}