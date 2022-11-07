#ifndef HTTPD_H
#define HTTPD_H

#include <string>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <dirent.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <semaphore.h>
#include <fcntl.h>
#include <signal.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <limits.h>
#include <cstring>
#include <netdb.h>
#include <map>
#include <algorithm>
#include "plugin.h"
#include "tpool.h"
#include "ban.h"
#include "timer.h"

using namespace std;


#define HTTP_VERSION "HTTP/1.1"
#define SERVER_NAME "Server: linrtyhttpd/0.1.0"
#define CONTENT_TYPE_TEXT "Content-Type: text/html"
#define CONTENT_TYPE_JPEG "Content-Type: image/jpeg"
#define CONTENT_TYPE_PNG "Content-Type: image/png"



#define MAXLINE 255
#define MAX_BUFFER_LENGTH 1024
#define EPOLL_SIZE 1024
#define FD_SIZE 1024

const string error_path = "./error";
const string default_file = "index.html";
const string separator = "/";
const string space = " ";

const int CODE_BAD_REQUEST = 400;
const int CODE_NOT_FOUND = 404;
const int CODE_FORBIDDEN = 403;
const int CODE_METHOD_NOT_IMPLEMENTED = 501;
const int CODE_INTERNAL_SERVER_ERROR = 500;

typedef struct request_info recv_info;


void error_log(char * error_string, int client_id);

// 创建socket
int create_socket(int* p_sockfd, int port);

// 接收请求并响应
void *socket_recv_request(void* args);

// 开启进程监听客户端的连接
void start_httpd(unsigned short port, char *doc_root, int maxnum_thread);

// 成功响应的头部
void response_ok_head(int client_id, string file_type);

// 客户端请求文件
bool request_file(recv_info info, int client_id);

// get响应
bool response_get_request(recv_info info, int client_id);

// post响应
bool response_post_request(recv_info info, int client_id);

// 响应错误
void response_error(int client_id, int error_code);

// 执行cgi脚本
bool exec_cgi(recv_info info, int client_id);

// 关闭与客户端的连接
void close_client(int epfd, int fd);

void get_client_link(int sockfd, int epfd);


#endif // HTTPD_H
