#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <cstdio>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#define BUF_SIZE 64
//client
int main(int argc, char* argv[]) {
	if (argc <= 2)
	{
		printf("usage: %s ip host\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);//字符串转整型
	sockaddr_in address;//tcp/ip专用socket地址
	bzero(&address, sizeof(address));//bzero与memset效果相同，bzero是linux特有的函数
	//memset(&address, '\0', sizeof(address));
	address.sin_family = AF_INET;//设置地址族
	address.sin_port = htons(port);//htons:host to net short 字节序转换
	inet_pton(AF_INET, ip, &address.sin_addr);//ip地址转换函数，反过来是inet_ntop

	int ret;
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	ret = connect(sockfd, (sockaddr*)&address, sizeof(address));
	if (ret < 0)
	{
		printf("connection failure!errno:%d\n", errno);
		return 1;
	}
	int p[2];
	ret = pipe(p);
	assert(ret != -1);
	pollfd fds[2];
	fds[0].fd = 0;
	fds[0].events = POLLIN;
	fds[0].revents = 0;
	fds[1].fd = sockfd;
	fds[1].events = POLLIN | POLLRDHUP;
	fds[1].revents = 0;
	char msg[BUF_SIZE];
	while (1)
	{
		ret = poll(fds, 2, -1);//返回就绪（可读、可写、异常）的文件描述符的总数，返回-1表示失败并设置了errno
		if (ret < 0)
		{
			printf("poll failure!\n");
			break;
		}
		//轮询每个文件描述符与事件
		if (fds[1].revents & POLLRDHUP)//服务器关闭了socket连接
		{
			printf("connection closed by server!\n");
			break;
		}
		else if (fds[1].revents & POLLIN)//socket端口有输入
		{
			memset(msg, '\0', BUF_SIZE);
			ret = recv(fds[1].fd, msg, BUF_SIZE - 1, 0);
			if (ret < 0)
			{
				if (errno != EAGAIN)
				{
					printf("Recv error!\n");
					continue;
				}
			}
			printf("%s", msg);
		}
		if (fds[0].revents & POLLIN)//用户直接输入终端
		{
			//splice()函数可以在两个文件描述符之间移动数据，且其中一个描述符必须是管道描述符
			splice(0, NULL, p[1], NULL, 32768, SPLICE_F_MORE); //输入零拷贝到管道
			splice(p[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE);//管道零拷贝到socket
		}
	}
	close(sockfd);
	return 0;
}