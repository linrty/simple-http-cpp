#include<stdio.h>
#include<stdlib.h>
#include <time.h>

int main()
{
	printf("Server: linrtyhttp/0.1.0\r\n");
    printf("Content-Type:text/html;\r\n\r\n");
	printf("<html>");
	printf("<head>");
	printf("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />");
	printf("<title> Info </title>");
	printf("</head>");
	printf("<body>");
	printf("<p>学号:202234261005</p>");
	printf("<p> 姓名:林杰</p>");
	printf("<p> 时间:</p>");
	
	time_t t;
	time(&t);
	char *time_ch = ctime(&t);

	printf("%s",time_ch);
	printf("</body>");
	printf("</html>");
	return 0;
}