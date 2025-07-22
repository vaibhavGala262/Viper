#ifndef ROUTER_H
#define ROUTER_H

#include "route_manager.h" 

void init_routes(void);       // adds dynamic routes with path params
int check_route_with_method(const char *path, const char *method, RouteParam *params, Route **matched_route);

#endif