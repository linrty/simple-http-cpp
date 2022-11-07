#include "httpd.h"

using namespace std;



void usage(char * argv0){
	cerr << "Usage: " << argv0 << " listen_port docroot_dir" << endl;
}

int main(int argc, char *argv[]){

    if (argc != 4){
        printf("缺少参数!!\n");
        return 0;
    }
    // 解析端口
	unsigned short port = strtol(argv[1], NULL, 10);

	// 指定线程池最大数量
	int maxnum_thread = strtol(argv[3], NULL, 10);
	
	if (errno == EINVAL || errno == ERANGE) {
		usage(argv[0]);
		return 2;
	}

	if (port <= 0 || port > USHRT_MAX) {
		cerr << "Invalid port: " << port << endl;
		return 3;
	}

	// 资源文件目录
	char *doc_root = argv[2];

	start_httpd(port, doc_root, maxnum_thread);

	return 0;
}
