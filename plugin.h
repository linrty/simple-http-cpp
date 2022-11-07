#ifndef PLUGIN_H
#define PLUGIN_H

#include<sys/epoll.h>
#include<assert.h>
#include<fcntl.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<errno.h>
#include<stdio.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include <string>
#include<stdarg.h>
#include<algorithm>
#include<map>
#include <sys/mman.h>
#include<stdlib.h>
#include<cstdint>
#include<new>
#include "httpd.h"

using namespace std;

typedef struct request_info{
    string uri;
    bool is_get;
    string version;

    map<string, string>headers;
    bool flag;
    map<string, string>body;
    map<string, string>query;
    string query_string;
}recv_info;

// 设置描述符为非阻塞状态
int set_nonblocking(int fd);

// 解析请求内容
recv_info parse_recv_info(int client_id);

// 读取一行
string read_line(int client_id);

// 发送一行
void write_line(int client_id, string str);

// 根据路径获取文件类型
string get_file_type(string file_path);

// 发送文件
void cat_file(int client_id, string file_path);

size_t writen(int fd, void *usrBuf, size_t n);

#endif
