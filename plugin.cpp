#include "plugin.h"


int set_nonblocking(int fd){
    // 获取描述符状态
    int flag = fcntl(fd,F_GETFL);
	if (flag == -1){
        printf("get flag error!\n");
        exit(0);
    }
    // 设置为非阻塞
	flag = flag | O_NONBLOCK;
	int ret = fcntl(fd,F_SETFL,flag);
	if (ret == -1){
        printf("set flag error!\n");
        exit(0);
    }
	return flag;
}

string read_line(int client_id){
	string str = "";
	char ch;
	while(true){
		int res = recv(client_id, &ch, 1, MSG_WAITALL);
		if (res <= 0 ){
            // 没有数据可以读的了
            break;
        }
		if (ch == '\r'){
			// 读取套接字中的字符到缓冲区，查看是不是\n
            str += ch;
			res = recv(client_id, &ch, 1, MSG_PEEK);
			if (res > 0 && ch == '\n'){
				// 一行的结束标志
				res = recv(client_id, &ch, 1, MSG_WAITALL);
				str += ch;
			}else{
				// 这一行没有正常的结束
				str += '\n';
			}
			// 结束一行
			break;
		}
		// 其他情况代表正常字符
		str += ch;
	}
	return str;
}

void write_line(int client_id, string str){
	str += "\r\n";
	const char *p_str = NULL;
	p_str = str.c_str();
	send(client_id, p_str, strlen(p_str), 0);
}


void clear_fd_recv(int client_id){
	while(true){
		string str = read_line(client_id);
		if (str == ""){
			break;
		}
	}
}



recv_info parse_recv_info(int client_id){
	recv_info info;
    info.flag = true;
	string line_str;
	// 先读取第一行，包含了version，method，uri信息
    do{
        line_str = read_line(client_id);
    } while (line_str == "\r\n");
	if (line_str == ""){
        // 读不到任何东西
        info.flag = false;
        clear_fd_recv(client_id);
        return info;
    }
	// 处理第一行的信息
	int i = 0;
	int pos = 0;
	int len = line_str.size();
	string str = "";
	while(i < len){
		if (line_str[i] == ' ' || line_str[i] == '\r'){
			// 遇到分割
			if (pos == 0){
				// method
				const char *p_str = NULL;
				p_str = str.c_str();
				if (!strcasecmp("get",p_str)){
					// get请求
					info.is_get = true;
				}else if (!strcasecmp("post",p_str)){
					// post请求
					info.is_get = false;
				}else{
					// 返回错误
                    info.flag = false;
                    clear_fd_recv(client_id);
                    return info;
				}
				pos++;
			}else if (pos == 1){
				// uri
				info.uri = str;
				pos++;
			}else{
				// version
				info.version = str;
				break;	
			}
			str = "";
		}else{
			str += line_str[i];
		}
		i++;
	}
	
	// 处理header
	while(true){
		line_str = read_line(client_id);
		if(line_str == "/r/n" || line_str == ""){
			// 遇到了header的结束
			break;
		}
		int len = line_str.size();
		i = 0;
		string key = "";
		while(i < len){
			if (line_str[i] == ':'){
				// 碰到第一个冒号表示key已经结束,将key中的字母全部转换成小写
				transform(key.begin(),key.end(),key.begin(),::tolower);
				string value = "";
				// 读取value值
				i++;
				// 找第一个非空格字符
				while(i < len && line_str[i] == ' ') i++;
				while(i < len){
					if (line_str[i] == '\r'){
						// value结束了，保存至map中
						info.headers[key] = value;
						break;
					}else{
						value += line_str[i];
					}
					i++;
				}
				break;
			}else{
				key += line_str[i];
			}
			i++;
		}
	}
	info.query.clear();
    info.body.clear();
	// 如果是post，继续处理body
	// 如果是get，处理query
	if(info.is_get){
		// 处理query
		// query值已经被放在uri中了
		string uri = info.uri;
		int len = uri.size();
		i = 0;
		// 找？
		while(i < len && uri[i] != '?') i++;
		if (i < len){
			// 存在？，即有query
			int pos = i;
			i++;
			string key = "";
			string value = "";
			bool is_key = true;
			while(i < len){
				if (uri[i] == '&'){
					// 遇到分隔符
					info.query[key] = value;
					is_key = true;
				}else if(uri[i] == '='){
					// key已经结束
					is_key = false;
				}else{
					// 普通字符
					if (is_key){
						key += uri[i];
					}else{
						value += uri[i];
					}
				}
				i++;
			}
			// 加最后一对key value
			info.query[key] = value;
			// 去除uri中？后面的部分
			info.uri = uri.substr(0,pos);
            info.query_string = uri.substr(pos + 1);
		}
	}else{
		// 处理body
		// 继续读行进行处理
	}
    return info;
}

string get_file_type(string file_path){
    int len = file_path.length();
    len--;
    while(len >= 0 && file_path[len] != '.' && file_path[len] != '/'){
        len--;
    }
    if (len < 0 || file_path[len] == '/'){
        // 没有找到扩展名
        printf("file name error!\n");
        return "";
    }
    len ++;
    string res = "";
    while(file_path[len]){
        res += file_path[len];
        len++;
    }
    return res;
}

void cat_file(int client_id, string file_path){
    int fd = open(file_path.c_str(), O_RDONLY, 0);
    ftruncate(fd, 20);
    int len = lseek(fd, 0, SEEK_END);
    char *srcAddr = (char *)mmap(NULL, len, PROT_READ, MAP_PRIVATE, fd, 0);
    close(fd);
    int send_len = writen(client_id, srcAddr, len);
}

size_t writen(int fd, void *usrBuf, size_t n) {
    size_t nleft = n;
    ssize_t nwritten;
    char *bufp = (char *)usrBuf;

    while(nleft > 0){
        if((nwritten = write(fd, bufp, nleft)) <= 0){
            if (errno == EINTR)
                nwritten = 0;
            else{
                return -1;
            }
        }
        nleft -= nwritten;
        bufp += nwritten;
    }
    return n;
}
