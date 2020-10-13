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

int main(int argc, char* argv[]) {
	if (argc <= 2)
	{
		printf("usage: %s ip host\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);
	sockaddr_in address;
	bzero(&address, sizeof(address));
	//memset(&address, '\0', sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_pton(AF_INET, ip, &address.sin_addr);

	int ret;
	int sockfd = socket(PF_INET, SOCK_STREAM, 0);
	ret = connect(sockfd, (sockaddr*)&address, sizeof(address));
	if (ret<0)
	{
		printf("connection failure!errno:%d\n", errno);
		return 1;
	}
	int p[2];
	ret=pipe(p);
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
		ret = poll(fds, 2, -1);
		if (ret<0)
		{
			printf("poll failure!\n");
			break;
		}
		if (fds[1].revents&POLLRDHUP)
		{
			printf("connection closed by server!\n");
			break;
		}
		else if(fds[1].revents&POLLIN)
		{
			memset(msg, '\0', BUF_SIZE);
			ret = recv(fds[1].fd, msg, BUF_SIZE-1, 0);
			if (ret<0)
			{
				if (errno!=EAGAIN)
				{
					printf("Recv error!\n");
					continue;
				}
			}
			printf("%s", msg);
		}
		if (fds[0].revents&POLLIN)
		{
			splice(0, NULL, p[1], NULL, 32768, SPLICE_F_MORE);
			splice(p[0], NULL, sockfd, NULL, 32768, SPLICE_F_MORE);
		}
	}
	close(sockfd);
	return 0;
}