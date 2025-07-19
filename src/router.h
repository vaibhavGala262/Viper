#ifndef ROUTER_H
#define ROUTER_H

#include "route_manager.h" 

void resolve_path(char *path);  //STATIC paths like html , css , js , files or images
void init_routes(void);       // adds dynamic routes with path params
int check_api_route(const char *path, RouteParam *params); // check if route is api route (like has path params)

#endif