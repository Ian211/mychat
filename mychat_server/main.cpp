#include <cstdio>
#include <libgen.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <string>
#include <arpa/inet.h>
#include <assert.h>
#include <poll.h>
#include <unistd.h>
#define BUF_SIZE 64
#define USER_LIMIT 5
#define FD_LIMIT 65535
struct client
{
	sockaddr_in address;
	char* write;
	char buf[BUF_SIZE];
};
int main(int argc, char* argv[]) {
	if (argc <= 2)
	{
		printf("usage: %s ip host\n", basename(argv[0]));
		return 1;
	}
	const char* ip = argv[1];
	int port = atoi(argv[2]);

	sockaddr_in	address;
	bzero(&address, sizeof(address));
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	inet_pton(AF_INET, ip, &address.sin_addr);

	int ret;
	int listenfd = socket(PF_INET, SOCK_STREAM, 0);
	assert(listen >= 0);
	ret = bind(listenfd, (sockaddr*)&address, sizeof(address));
	assert(ret != -1);

	client* USERS = new client[FD_LIMIT];
	int user_count = 0;
	pollfd fds[USER_LIMIT + 1];
	fds[0].fd = listenfd;
	fds[0].events = POLLIN | POLLERR;
	fds[0].revents = 0;
	for (int i = 1; i < USER_LIMIT + 1; i++)
	{
		fds[i].fd = -1;
		fds[i].events = 0;
		fds[i].revents = 0;
	}
	ret = listen(listenfd, 5);
	assert(ret != -1);
	while (1)
	{
		ret = poll(fds, USER_LIMIT + 1, -1);
		if (ret < 0)
		{
			printf("poll failure!\n");
			break;
		}
		else if (ret == 0)
		{
			continue;
		}
		else
		{
			for (int i = 0; i < user_count + 1; i++)
			{
				if ((fds[i].fd == listenfd) && (fds[i].revents & POLLIN))
				{
					int clientfd;
					sockaddr_in clientAddr;
					bzero(&clientAddr, sizeof(clientAddr));
					socklen_t length = (socklen_t)sizeof(clientAddr);
					clientfd = accept(listenfd, (sockaddr*)&clientAddr, &length);
					if (clientfd < 0)
					{
						printf("errno:&d\n", errno);
						continue;
					}
					if (user_count >= USER_LIMIT)
					{
						const char* info = "Sorry,too many users\n";
						printf("%s", info);
						send(clientfd, info, sizeof(info), 0);
						close(clientfd);
						continue;
					}
					user_count++;
					USERS[clientfd].address = clientAddr;
					fds[user_count].fd = clientfd;
					fds[user_count].events = POLLIN | POLLRDHUP | POLLERR;
					fds[user_count].revents = 0;
					printf("comes a new user,now have %d users\n", user_count);
				}
				else if (fds[i].revents & POLLERR)
				{
					printf("get an error from %d\n", fds[i].fd);
					char errors[100];
					memset(errors, '\0', sizeof(errors));
					socklen_t length = sizeof(errors);
					if (getsockopt(fds[i].fd, SOL_SOCKET, SO_ERROR, &errors, &length) < 0)
					{
						printf("getsockopt failed,errno:%d\n", errno);
					}
					printf("error:%s\n", errors);
					continue;
				}
				else if (fds[i].revents & POLLRDHUP)
				{
					USERS[fds[i].fd] = USERS[fds[user_count].fd];
					printf("user %d left\n", fds[i].fd);
					close(fds[i].fd);
					fds[i].fd = fds[user_count].fd;
					user_count--;
					i--;
				}
				else if (fds[i].revents & POLLIN)
				{
					int clientfd = fds[i].fd;
					memset(USERS[clientfd].buf, '\0', BUF_SIZE);
					ret = recv(clientfd, USERS[clientfd].buf, BUF_SIZE, 0);
					if (ret < 0)
					{
						if (errno != EAGAIN)
						{
							USERS[fds[i].fd] = USERS[fds[user_count].fd];
							printf("user %d left for error:%d\n", fds[i].fd, errno);
							close(fds[i].fd);
							fds[i].fd = fds[user_count].fd;
							user_count--;
							i--;
						}
					}
					else if (ret == 0)
					{
						printf("code should not come to here\n");
					}
					else
					{
						printf("Get %d bytes from %d: %s ", ret, fds[i].fd, USERS[fds[i].fd].buf);
						for (int j = 1; j < user_count + 1; j++)
						{
							if (i == j)
								continue;
							fds[j].events |= ~POLLIN;
							fds[j].events |= POLLOUT;
							USERS[fds[j].fd].write = USERS[fds[i].fd].buf;
						}
					}
				}
				else if (fds[i].revents & POLLOUT)
				{
					if (!USERS[fds[i].fd].write)
					{
						continue;
					}
					ret = send(fds[i].fd, USERS[fds[i].fd].write, strlen(USERS[fds[i].fd].write), 0);
					if (ret < 0)
					{
						if (errno != EAGAIN)
						{
							USERS[fds[i].fd] = USERS[fds[user_count].fd];
							printf("user %d left for error:%d\n", fds[i].fd, errno);
							close(fds[i].fd);
							fds[i].fd = fds[user_count].fd;
							user_count--;
							i--;
						}
					}
					USERS[fds[i].fd].write = NULL;
					fds[i].events |= POLLIN;
					fds[i].events |= ~POLLOUT;
				}
			}
		}
	}
	delete USERS;
	close(listenfd);
	return 0;
}