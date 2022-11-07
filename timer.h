//
// Created by linrty on 2022/11/6.
//

#ifndef TIMER_H
#define TIMER_H

#include <queue>
#include <vector>
#include <algorithm>
#include<sys/time.h>
#include "httpd.h"


typedef struct timer{
    size_t key;
    int fd;
    bool operator < (const timer &a) const{
        return key > a.key;
    }
}timer_fd;

int create_timer();

void update_current_time();

int get_time();

timer_fd add_timer_2_queue(int fd, size_t timeout);

void delete_timer_from_queue(int fd);

void handle_expire_timer_node(int epfd);

#endif //TIMER_H
