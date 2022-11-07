#ifndef BAN_H
#define BAN_H

#include "httpd.h"

using namespace std;

typedef struct point{
    point *l_child;
    point *r_child;
    bool is_ban;
}P;

void init_ban_addr(bool is_ban, string ban_file_path, P *root);

bool is_ban_adrr(char *addr, P *root);

void ip_2_binary(char *addr, char *binary_ip);

void add_point(P *h, char *addr,int cidr,bool is_ban);


#endif