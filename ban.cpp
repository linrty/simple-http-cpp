#include "ban.h"


// 初始化禁止IP名单
void init_ban_addr(bool is_ban, string ban_file_path, P *root){
    // 通过resourse资源目录找禁止名单
    // 读取文件
    FILE *fp = NULL;
    fp = fopen(ban_file_path.c_str(), "r");
    char ip[MAXLINE];
    char words[MAXLINE];
    char binary_ip[MAXLINE];
    bool is_all = false;
    while(!feof(fp)){
        printf("start read file a line\n");
        int cidr = 0;
        // 读取第一个字符deny or allow
        fscanf(fp, "%s",words);
        // 读取from，无用可丢弃
        fscanf(fp, "%s",ip);
        // 读取ip
        fscanf(fp, "%s\r\n",ip);
        printf("%s  %s/n",words, ip);
        // 是否处理deny
        if (is_ban && (!strcasecmp("allow",words))){
            continue;
        }
        if (!is_ban && (!strcasecmp("deny",words))){
            continue;
        }
        int k = 0;
        while(ip[k] && ip[k] !='/') k++;
        if (ip[k] == '/'){
            // 是ip,处理末尾部分
            int p = k;
            p++;
            while(ip[p]){
                cidr = cidr * 10 + (ip[p] - '0');
                p++;
            }
            printf("%d\n", cidr);

            ip[k] = '\0';
        }else{
            // 是一个域名，转ip
            struct hostent *hptr;
            struct in_addr in;
            struct sockaddr_in addr_in;
            hptr = gethostbyname(ip);
            if(hptr == NULL){
                printf("get host by name error %s\n",ip);
                continue;
            }
            memcpy(&addr_in.sin_addr.s_addr,hptr->h_addr,4);
            in.s_addr=addr_in.sin_addr.s_addr;
            printf("解析成功： %s -> %s\n",ip,inet_ntoa(in));
            strcpy(ip, inet_ntoa(in));
            cidr = 32;
        }
        ip_2_binary(ip, binary_ip);
        if (!strcmp("0.0.0.0",ip)){
            // 检查是否是特殊ip
            is_all = true;
        }

        if (is_all){
            root->is_ban = is_ban;
        }else{
            add_point(root, binary_ip, cidr,is_ban);
        }
    }
    fclose(fp);
}

// 判断是否是禁止的IP
bool is_ban_adrr(char *addr, P *root){
    char binary_ip[MAXLINE];
    // 转化成二进制
    ip_2_binary(addr, binary_ip);
    P *node = NULL;
    node = root;
    int i = 0;
    while(binary_ip[i]){
        if (binary_ip[i] == '0'){
            if (node->l_child == NULL){
                // 走到了叶子节点
                return node->is_ban;
            }else{
                // 还未走到叶子节点，继续往下走
                node = node->l_child;
            }
        }else{
            if (node->r_child == NULL){
                return node->is_ban;
            }else{
                node = node->r_child;
            }
        }
        i++;
    }
    // 应该到不了这一步，因为一定会碰到叶子节点
    return true;
}

// 字符串形式的ip转成二进制形式
void ip_2_binary(char *addr, char *binary_ip){
    // 通过点将地址分成四部分十进制的整数
    int len = strlen(addr);
    int i = 0;
    int j = 0;
    int num[4] = {0};
    while(i < len){
        if (addr[i] == '.'){
            // 找到分割点
            j++;
        }else{
            // 加入到数字中
            num[j] = num[j] * 10 + (addr[i] - '0');
        }
        i++;
    }
    // 先填充0
    for(i = 0;i < 4*8;i++){
        binary_ip[i] = '0';
    }
    binary_ip[32] = '\0';
    // 将数字一个个取出并转成二进制
    for (j = 0 ;j < 4; j++){
        int n = num[j];
        // ip 0-255 需要8位2进制
        i = 7;
        // 从后往前填充
        while(n>0){
            binary_ip[j*8 + i] = (n%2) + '0';
            n/=2;
            i--;
        }
    }

}

void add_point(P *h, char *addr,int cidr,bool is_ban){
    if (cidr == 0){
        // 已经到尾部了
        h->is_ban = is_ban;
        // 清空子树，防止结构破坏
        h->l_child = NULL;
        h->r_child = NULL;
        return;
    }
    if (*addr == '0'){
        // 进左儿子
        printf("0");
        if(h->l_child == NULL){
            h->l_child = (P *)malloc(sizeof(P));
            // 默认和父亲一样
            h->l_child->is_ban = h->is_ban;
            // 指针初始化为NULL
            h->l_child->l_child = h->l_child->r_child = NULL;
        }
        add_point(h->l_child, addr + 1, cidr - 1, is_ban);
    }else{
        printf("1");
        // 进右儿子
        if(h->r_child == NULL){
            h->r_child = (P *)malloc(sizeof(P));
            // 默认和父亲一样
            h->r_child->is_ban = h->is_ban;
            // 指针初始化为NULL
            h->r_child->l_child = h->r_child->r_child = NULL;
        }
        add_point(h->r_child, addr + 1, cidr - 1, is_ban);
    }
}