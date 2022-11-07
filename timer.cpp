//
// Created by j1265 on 2022/11/6.
//

#include "timer.h"

size_t current_time;
priority_queue<timer_fd>timer_queue;
map<int,bool>deleted;

int create_timer(){
    update_current_time();
    deleted.clear();
    return 0;
}

void update_current_time(){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    current_time = ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

// 获取最早的事件节点和当前时间的差值
int get_time(){
    int time = 0;
    while(!timer_queue.empty()){
        update_current_time();
        timer_fd node = timer_queue.top();
        if (deleted[node.fd]){
            // 删除标记
            timer_queue.pop();
            continue;
        }

        time = (int)(node.key - current_time);
        time = (time > 0)? time : 0;
        break;
    }
    return time;
}

timer_fd add_timer_2_queue(int fd, size_t timeout){
    update_current_time();
    timer_fd node;
    node.key = current_time + timeout;
    node.fd = fd;
    deleted[node.fd] = false;
    timer_queue.push(node);
    return node;
}


void delete_timer_from_queue(int fd){
    update_current_time();
    // 惰性删除
    deleted[fd] = true;
}

void handle_expire_timer_node(int epfd){
    while(!timer_queue.empty()){
        update_current_time();
        timer_fd node = timer_queue.top();
        if (deleted[node.fd]){
            timer_queue.pop();
            continue;
        }
        if (node.key > current_time){
            // 因为是有序的，所以只需要查看第一个节点是否超时就可以判断整个队列是否超时
            return ;
        }
        // 超时处理
        printf("have node timeout %d\n",node.fd);
        close_client(epfd,node.fd);
        timer_queue.pop();
    }
}