#ifndef ROUTER_H
#define ROUTER_H

#include "route_manager.h" 

void init_routes(void);       // adds dynamic routes with path params
int check_route(const char *path, RouteParam *params, Route **matched_route);

#endif