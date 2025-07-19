#ifndef ROUTE_MANAGER_H
#define ROUTE_MANAGER_H

#include "pattern_matching.h"

#define MAX_ROUTES 32

typedef enum {
    ROUTE_TYPE_API, // Dynamic content generation
    ROUTE_TYPE_STATIC,  // File serving 
    ROUTE_TYPE_REDIRECT  // url redirects
} RouteType;  //stores all routers in structure via types 1) API , 2) STATIC , 3) REDIRECTS (Like redirect home)

typedef struct {
    char pattern[256];
    RouteType type;
    char handler_name[64];
} Route;    // has all patterns of API routes of our server to match new client requests

typedef struct {
    Route routes[MAX_ROUTES];
    int count;
} RouteManager;    // stores all routes

void init_route_manager(RouteManager *rm); 
int add_route(RouteManager *rm, const char *pattern, RouteType type, const char *handler_name);  // helps add API routes
int find_route(RouteManager *rm, const char *path, RouteParam *params, int max_params, Route **matched_route); // find if API routes exist

#endif