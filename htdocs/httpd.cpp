
#include "httpd.h"
#include "threadpool.h"


using namespace std;


string resource_dir;

P *root = NULL;

struct threadPool* pool = NULL;

map<int,string>fd_2_ip;

int epfd;

int fd_list[MAX_BUFFER_LENGTH];

int time_out;

void response_error(int client_id, int error_code){
	printf("error %d\n",error_code);
	string str = HTTP_VERSION;
	switch (error_code){
		case CODE_BAD_REQUEST:
			// 400
            str = str + space + to_string(error_code) + space + "BAD REQUEST";
			break;
		case CODE_FORBIDDEN:
			// 403
            str = str + space + to_string(error_code) + space + "FORBIDDEN";
			break;
		case CODE_NOT_FOUND:
			// 404
            str = str + space + to_string(error_code) + space + "NOT FOUND";
			break;
		case CODE_INTERNAL_SERVER_ERROR:
			// 500
            str = str + space + to_string(error_code) + space + "INTERNAL SERVER ERROR";
			break;
		case CODE_METHOD_NOT_IMPLEMENTED:
			// 501
            str = str + space + to_string(error_code) + space + "METHOD NOT IMPLEMENTED";
			break;
		default:
			break;
	}
    string error_file_path = error_path + separator + to_string(error_code) + ".html";
	FILE *error_file = NULL;
	error_file = fopen(error_file_path.c_str(), "r");
	if (error_file == NULL){
		printf("can not found or can not open the error file\n");
		return;
	}
	else{
        fclose(error_file);
        write_line(client_id, str);
        write_line(client_id, SERVER_NAME);
        // text/html
        write_line(client_id, CONTENT_TYPE_TEXT);
        // 空行
        write_line(client_id, "");
		cat_file(client_id, error_file_path);
	}
	printf("response error file successfully!\n");
}


void error_log(char * error_string, int client_id){
	if (client_id != -1){
		printf("client: %d, error: %s\n",client_id,error_string);
	}else{
		printf("%s\n",error_string);
	}
	
}


void response_ok_head(int client_id, string file_type){
	string str = "";
	str = str + HTTP_VERSION +  space + "200" + space + "OK";
	write_line(client_id, str);
    str = SERVER_NAME;
    write_line(client_id, str);
    str = "Connection: keep-alive";
    write_line(client_id, str);
    str = "Keep-Alive: timeout=5000";
    write_line(client_id, str);

    if (file_type == "html") {
        str = CONTENT_TYPE_TEXT;
    }else if (file_type == "png"){
        str = CONTENT_TYPE_JPEG;
    }else if (file_type == "jpg"){
        str = CONTENT_TYPE_PNG;
    }
    write_line(client_id, str);
    write_line(client_id, "");
}


bool request_file(recv_info info, int client_id){
	FILE *file = NULL;
	string uri = info.uri;
	string file_path = resource_dir;

	if (uri == "/"){
		// 直接进首页
		file_path = file_path + separator + default_file;
	}else{
		// 文件路径
		file_path = file_path + uri;
	}

	// 查看是文件还是目录
	struct stat st;
	stat(file_path.c_str(), &st);

	if(S_ISDIR(st.st_mode)){
		// 目录
		file_path = file_path + separator + default_file;
	}

	file = fopen(file_path.c_str(), "r");
	if (file == NULL){
		response_error(client_id, CODE_NOT_FOUND);
        return true;
	}else{
        fclose(file);
		string file_type = get_file_type(file_path);
		if (file_type == "html" || file_type == "png" || file_type == "jpg"){
            // 发送头
            response_ok_head(client_id, file_type);
            // 发送文件本体
            cat_file(client_id, file_path);
		}else{
			// 暂不支持的文件类型
			response_error(client_id, CODE_METHOD_NOT_IMPLEMENTED);
            return true;
		}
	}
	printf("response a file successfully\n");
    return false;
}


// 创建socket
int create_socket(int* p_sockfd, int port){
	// struct sockaddr_in{ 
    //          unsigned short          sin_family;  // 一般设为AF_INET   
    //          unsigned short int      sin_port;    // 监听的端口号
    //          struct in_addr          sin_addr;    // 一般设为 INADDR_ANY表示可以和任何主机通信
    //          unsigned char           sin_zero[8]; // 用来填充的
    struct sockaddr_in server_addr;

	// int socket(int domain, int type,int protocol)
	// AF_INET是争对Internet的，AF_UNIX只针对unix系统进程间的通信
	// SOCK_STREAM -> TCP 顺序的,可靠,双向,面向连接的比特流
	// SOCK_DGRAM  -> UDP 定长的,不可靠,无连接的通信
	*p_sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(*p_sockfd == -1){
		// socket创建失败
		printf("create socket error : %s \r\n",strerror(errno));
		return -1;
	}

    // 设置套接字地址可以重复使用
    int opt_val = 1;
    if (setsockopt(*p_sockfd, SOL_SOCKET, SO_REUSEADDR,
                   (const void *)&opt_val, sizeof(opt_val)) == -1){
        printf("setsockopt error!\n");
        return -1;
    }
    
	// 服务器端填充sockaddr结构
    bzero(&server_addr,sizeof(struct sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	server_addr.sin_port = htons(port);

    // 捆绑sockfd描述符
	if(bind(*p_sockfd,(struct sockaddr *)(&server_addr),sizeof(server_addr)) == -1){
		printf("bind tcp socket fail, err[%s]!!!\n", strerror(errno));
		close(*p_sockfd);
		*p_sockfd = 0;
		return -1;
	}

	// 监听sockfd描述符
	// int listen(int sockfd,int backlog) 
	// backlog:设置请求排队的最大长度.当有多个客户端程序和服务端相连时, 使用这个表示可以介绍的排队长度
	if(listen(*p_sockfd, 300) == -1){
		printf("socketCreate::listen tcp socket fail,err[%s]!!!\r\n",strerror(errno));
		close(*p_sockfd);
		return -1;
	}

    return 0;
}


//接收请求并进行响应
void socket_recv_request(void* args){
    bool is_error = false;
	int client_id = *(int*) args;

    // 删除定时器
    delete_timer_from_queue(client_id);
	// 处理描述符内的信息，进行格式化
	recv_info info = parse_recv_info(client_id);


	if (!info.flag){
		// 信息接收失败
        printf("parse info fail!\n");
        // 断开连接
        is_error = false;
	}else{
        // 查看是否在拦截名单里
        // 拦截相应的IP
        char ip[MAXLINE];
        strcpy(ip, fd_2_ip[client_id].c_str());
        bool is_ban = is_ban_adrr(ip, root);
        if(is_ban) {
            // 被禁止访问的IP
            response_error(client_id, CODE_FORBIDDEN);
            // 断开连接
            is_error = true;
            return ;
        }

        // 开始处理响应
        if (info.is_get){
            // get请求
            is_error = response_get_request(info,client_id);
        }else{
            // post请求，直接执行cgi
            is_error = response_post_request(info, client_id);
        }

        // 判断connection是否是keep-alive决定是否长连接
        if (strcasecmp(info.headers["connection"].c_str(),"keep-alive")){
            // 不是长连接
            is_error = true;
        }
    }

    // 判断是否出错来决定是否断开连接
    if (is_error){
        // 断开连接
        close_client(epfd, client_id);
    }else{
        // 修改epoll
        struct epoll_event event;
        event.data.fd = client_id;
        event.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
        if (epoll_ctl(epfd, EPOLL_CTL_MOD, client_id, &event) == -1){
            printf("epoll modify error.\n");
        }
        // 添加定时器
        add_timer_2_queue(client_id, time_out);
    }

	return ;
}



bool response_get_request(recv_info info, int client_id){
	map<string, string>query = info.query;

	if(info.query.size() > 0){
		// 带query的请求，直接执行cgi程序进行响应
        printf("hava %d query!\n",(int)info.query.size());
		return exec_cgi(info, client_id);
	}else{
		// 其他情况表示get请求后面不带参数
		// 比如：
		// /a.html   ------ 请求资源文件类
		// /get_info.cgi   ------- 不带参数的请求
		// 规定存放在项目根目录的/cgi目录下为cgi程序，对应的请求需要为 /get_info.cgi
		// 获取后缀
		string extend = get_file_type(info.uri);
		if(extend == "cgi"){
			// 执行cgi
			return exec_cgi(info, client_id);
		}else{
			// 返回资源文件
            return request_file(info, client_id);
		}
	}
}

bool response_post_request(recv_info info, int client_id){
	// post请求直接执行cgi
    return exec_cgi(info, client_id);
}


bool exec_cgi(recv_info info, int client_id){
	printf("start exec cgi file\n");
    string cgi_file_path;
	int output[2];
	int input[2];
	int status;
    int content_length = -1;
	
	// init cgi file path
    cgi_file_path = resource_dir + info.uri;

    //检查一下文件是否存在
    FILE *file = NULL;
    file = fopen(cgi_file_path.c_str(), "r");
    if (file == NULL){
        response_error(client_id, CODE_NOT_FOUND);
        return true;
    }
    fclose(file);

    // 创建通道
    if(pipe(output) < 0 || pipe(input) < 0){
        // 500
        response_error(client_id, CODE_INTERNAL_SERVER_ERROR);
        return true;
    }

    if (!info.is_get){
        // get请求执行cgi
        bool flag = false;

        for (map<string, string>::iterator iter = info.headers.begin(); iter != info.headers.end(); ++iter) {
            if (iter->first == "content-length"){
                flag = true;
            }
        }
        if (flag){
            // 存在Content-Length
            content_length = atoi(info.headers["content-length"].c_str());
        }else{
            // 不存在Content-Length
            response_error(client_id, CODE_BAD_REQUEST);
            return true;
        }
    }

	// 先返回正确码
    write_line(client_id, HTTP_VERSION + space +to_string(200) + space + "OK");

    printf("cgi file : %s , content-length: %d\n",cgi_file_path.c_str(),content_length);
    // 创建子线程执行cgi
	pid_t pid;
	pid = fork();
	if (pid < 0){
		// 线程创建失败
		printf("thread create fail!\n");
		response_error(client_id, CODE_BAD_REQUEST);
		return true;
	}else if (pid == 0){
		// 子线程
		dup2(output[1],1);
		dup2(input[0],0);
		close(output[0]);
		close(input[1]);
        string str = "REQUEST_METHOD=";
        if (info.is_get){
            str += "GET";
        }else{
            str += "POST";
        }
        char c_str[MAX_BUFFER_LENGTH];
        strcpy(c_str, str.c_str());
		putenv(c_str);
		// 初始化环境变量
		if (info.is_get){
			// get
            str = "QUERY_STRING=" + info.query_string;
            strcpy(c_str, str.c_str());
            putenv(c_str);
		}else{
			// post
            str = "CONTENT_LENGTH=" + to_string(content_length);
            strcpy(c_str, str.c_str());
            putenv(c_str);
		}
		// 执行CGI程序
		printf("start execl function\n");
        string file_name = "";
        int i = cgi_file_path.length() - 1;
        while(i>=0 && cgi_file_path[i]!='/') i--;
        file_name = cgi_file_path.substr(i + 1);
		int res = execl(cgi_file_path.c_str(), file_name.c_str(),NULL);
		printf("cgi result %d\n",res);
		// 退出子进程
		exit(0);
	}else{
		// 父线程
		close(output[1]);
  		close(input[0]);
		char ch;
		if (!info.is_get){
			// post
			for (int i = 0; i < content_length; i++){
				recv(client_id, &ch, 1, 0);
				write(input[1], &ch, 1);
			}
		}
		while(read(output[0], &ch, 1) > 0){
			send(client_id, &ch, 1, 0);
		}
		close(output[0]);
		close(input[1]);
		waitpid(pid, &status, 0);
	}
    return false;

}

void deal_event(struct epoll_event *events, int len, int sockfd, int epfd){

	for (int i=0 ;i < len; i++){

		int fd = events[i].data.fd;
        fd_list[i] = fd;
		if (fd == sockfd){
			// 监听到有新的客户端发来新连接的请求
            get_client_link(sockfd, epfd);
		}else{
            // 排除错误状态
            if((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
               || !(events[i].events & EPOLLIN)) {
                close(fd);
                continue;
            }
			// 客户端发来数据请求
			// 用线程池内的线程处理请求
            addThreadPool(pool, socket_recv_request, (&fd_list[i]));
		}
	}
}

void close_client(int epfd, int fd){
    printf("epfd : %d\n",epfd);
    printf("close %d\n",fd);
    close(fd);
}

void get_client_link(int sockfd, int epfd){
    struct sockaddr_in client_addr;
    socklen_t sin_size;
    sin_size = sizeof(struct sockaddr_in);
    int connfd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
    if (connfd < 0){
        printf("client link error!\n");
        return;
    }
    char str[MAXLINE];
    char ip[MAXLINE];
    strcpy(ip, inet_ntop(AF_INET, &client_addr.sin_addr, str, sizeof(str)));
    fd_2_ip[connfd] = string(ip);
    struct epoll_event ev;
    ev.data.fd=connfd;
    // EPOLLIN 触发事件
    // EPOLLRDHUP 断开连接事件
    // EPOLLET ET模式，边缘触发
    // EPOLLONESHOT 保证同一套接字只能被一个线程处理，不会跨越多个线程
    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLET | EPOLLONESHOT;
    // 设置套接字为非阻塞
    set_nonblocking(connfd);
    if(epoll_ctl(epfd,EPOLL_CTL_ADD,connfd,&ev)==-1){
        printf("epoll_ctl add error\n");
    }
    // 添加新的计时器
    add_timer_2_queue(connfd,time_out);
}




void start_httpd(unsigned short port, char *doc_root, int maxnum_thread){
	cerr << "Starting server (port: " << port <<
		", doc_root: " << doc_root << ")" << endl;
    resource_dir = string(doc_root);

	// 初始化线程池
//	if (0 != create_tpool(&pool, maxnum_thread)){
//        printf("error: Create thread pool failed!\n");
//		exit(1);
//	}
    pool = createThreadPool(maxnum_thread);
	
	printf("thread pool init successfully, the size of thread pool is %d\n",maxnum_thread);

    // 初始化定时器
    create_timer();
    // 5s
    time_out = 5000;
    printf("init timer successfully.\n");

	// 先初始化允许的再初始化禁止的
    string ban_file_path = resource_dir + separator + ".htaccess";
    root = (P *)malloc(sizeof(P));
    root->is_ban = true;
    root->l_child = root->r_child = NULL;
	init_ban_addr(false, ban_file_path, root);
	init_ban_addr(true, ban_file_path, root);

	printf("the tree of deny and allow ip is init successflly!\n");

	int my_fd = 0;
    if (0 != create_socket(&my_fd, port)){
        printf("error: Create Socket Error!\n");
		exit(1);
    }
	printf("web server running on port %d , resource files dir: %s\n",port,doc_root);


	// 创建内核事件表
	epfd = epoll_create1(0);
	if (epfd == -1){
		printf("epoll create fail!\n");
		exit(1);
	}

	struct epoll_event ev;
	ev.data.fd = my_fd;
	ev.events = EPOLLIN;

	// 注册
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, my_fd, &ev) == -1){
		printf("epoll_ctl add fail!\n");
		exit(1);
	}

	// 非阻塞接收请求
	while(true){
        // 更新一下时间
        get_time();
		struct epoll_event events[FD_SIZE];
		int len = epoll_wait(epfd, events, FD_SIZE, 500);
        // 先处理一下超时的连接，超时断开，释放套接字
        handle_expire_timer_node(epfd);

		if (len < 0){
			printf("epoll wait error!\n");
			exit(1);
		}else if (len == 0){
			printf("listen.....\n");
			// 没有请求
			continue;
		}else{
			// 有请求出现了
            deal_event(events, len, my_fd, epfd);
		}
	}

    freeThreadPool(pool);
    close(my_fd);
}



