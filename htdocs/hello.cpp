#include<stdio.h>
#include<stdlib.h>

int main()
{
	printf("Server: jdbhttpd/0.1.0\r\n");
    printf("Content-Type:text/html;\r\n\r\n");
	printf("<html>");
	printf("<head>");
	printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
	printf("</head>");
    printf("<body>hello world, 你好</body></html>");
	return 0;
}