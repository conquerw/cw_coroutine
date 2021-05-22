#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <cw_thread.h>
#include <cw_coroutine.h>
#include <cw_scheduler.h>
#include <cw_schedulers.h>
#include <httpsession.h>

#define MAX_CLIENT_NUM			1000000
#define TIME_SUB_MS(tv1, tv2)  ((tv1.tv_sec - tv2.tv_sec) * 1000 + (tv1.tv_usec - tv2.tv_usec) / 1000)

void server_reader(void *arg)
{
	cw_enable_hook_syscall();
	
	int fd = *(int *)arg;
	int ret = 0;
	
	while(1)
	{
		char buf[1024] = {0};
		ret = recv(fd, buf, sizeof(buf), 0);
		if(ret > 0)
		{
			http_request_t *http_request = (http_request_t *)calloc(1, sizeof(http_request_t));
			http_parse_msg(http_request, buf, ret);
			
			char buffer[1024] = "";
			memset(buffer, 0x00, sizeof(buffer));
			int buffer_len = 0;
			http_handle(buffer, &buffer_len, http_request);
			send(fd, buffer, buffer_len, 0);
			if(ret < 0)
			{
				close(fd);
				break;
			}
		}
		else if (ret == 0) 
		{	
			close(fd);
			break;
		}
	}
}

static int SetNonBlock(int iSock)
{
    int iFlags;

    iFlags = fcntl(iSock, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    int ret = fcntl(iSock, F_SETFL, iFlags);
    return ret;
}

void server(void *arg)
{
	cw_scheduler_t *cw_scheduler = (cw_scheduler_t *)arg;
	
	cw_enable_hook_syscall();
	
	unsigned short port = 50000;

	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd < 0) return ;
	
	int opt = 1;
	int ret = setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt));
	if(ret < 0)
	{
		printf("setsockopt\n");
		return;
	}
	
	ret = setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	if(ret < 0)
	{
		printf("setsockopt\n");
		return;
	}
	
	struct sockaddr_in local, remote;
	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = INADDR_ANY;
	bind(fd, (struct sockaddr*)&local, sizeof(struct sockaddr_in));

	SetNonBlock(fd);
	
	listen(fd, 4096);
	
	struct timeval tv_begin;
	gettimeofday(&tv_begin, NULL);

	while(1)
	{
		socklen_t len = sizeof(struct sockaddr_in);
		int cli_fd = accept(fd, (struct sockaddr*)&remote, &len);
		if (cli_fd % 1000 == 999)
		{
			struct timeval tv_cur;
			memcpy(&tv_cur, &tv_begin, sizeof(struct timeval));
			
			gettimeofday(&tv_begin, NULL);
			// int time_used = TIME_SUB_MS(tv_begin, tv_cur);
			
			// printf("client fd : %d, time_used: %d\n", cli_fd, time_used);
		}
		// printf("new client comming client fd: %d\n", cli_fd);
		SetNonBlock(cli_fd);
		cw_coroutine_t *cw_coroutine = (cw_coroutine_t *)calloc(1, sizeof(cw_coroutine_t));
		cw_coroutine_create(cw_scheduler, cw_coroutine, server_reader, &cli_fd);
	}
}

int main(int argc, char *argv[])
{
	cw_schedulers_t *cw_schedulers = cw_schedulers_init();
	
	for(int i = 0; i < cw_schedulers->cw_scheduler_len; i++)
	{
		cw_coroutine_t *cw_coroutine = (cw_coroutine_t *)calloc(1, sizeof(cw_coroutine_t));
		cw_coroutine_create(cw_schedulers->cw_scheduler + i, cw_coroutine, server, cw_schedulers->cw_scheduler + i);
	}
	
	cw_schedulers_run();
	
	while(1)
		sleep(10);
	
	return 1;
}
