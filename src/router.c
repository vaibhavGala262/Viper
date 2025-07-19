#include <stdio.h>
#include <string.h>
#include "router.h"

#include <string.h>
#include <stdio.h>
#include "router.h"

void resolve_path(char *path) {
    if (strcmp(path, "/") == 0) {
        strcpy(path, "/html/index.html");
    } else if (strcmp(path, "/trail") == 0) {
        strcpy(path, "/html/trail.html");
    } else if (strcmp(path, "/css/style.css") == 0) {
        strcpy(path, "/css/style.css");
    } else if (strcmp(path, "/images/roman") == 0) {
        strcpy(path, "/images/image4.jpg");
    }else if(strcmp(path , "/downloads/result")==0){
        strcpy(path , "/downloads/result.pdf");
    }
    // else leave it unchanged, assumes /html/actual.html or /css/xyz.css requested directly
}
