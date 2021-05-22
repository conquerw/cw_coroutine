/************************************************
 * @addtogroup CITS
 * @{
 * @file  : cw_hook_syscall.c
 * @brief : 
 * @author: wangshaobo
 * @email : wangshaobo@nebula-link.com
 * @date  : 2019-05-10 10:43:51
************************************************/

//-----------------------------------------------
// Copyright (c) CITS
//-----------------------------------------------

#include <errno.h>
#include <dlfcn.h>
#include <poll.h>
#include <string.h>
#include <sys/socket.h>

#define HOOK_SYS_FUNC(name) if(!g_sys_##name##_func) {g_sys_##name##_func = (name##_pfn_t)dlsym(RTLD_NEXT, #name);}

typedef int (*socket_pfn_t)(int domain, int type, int protocol);
typedef int (*close_pfn_t)(int fd);

typedef int (*connect_pfn_t)(int socket, const struct sockaddr *address, socklen_t address_len);
typedef int (*accept_pfn_t)(int socket, struct sockaddr *address, socklen_t *address_len);

typedef ssize_t (*send_pfn_t)(int socket, const void *buffer, size_t length, int flags);
typedef ssize_t (*recv_pfn_t)(int socket, void *buffer, size_t length, int flags);

typedef ssize_t (*sendto_pfn_t)(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len);
typedef ssize_t (*recvfrom_pfn_t)(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len);

typedef ssize_t (*write_pfn_t)(int fildes, const void *buf, size_t nbyte);
typedef ssize_t (*read_pfn_t)(int fildes, void *buf, size_t nbyte);

typedef int (*poll_pfn_t)(struct pollfd fds[], nfds_t nfds, int timeout);

static socket_pfn_t g_sys_socket_func = NULL;
static close_pfn_t g_sys_close_func = NULL;

static connect_pfn_t g_sys_connect_func = NULL;
static accept_pfn_t g_sys_accept_func = NULL;

static send_pfn_t g_sys_send_func = NULL;
static recv_pfn_t g_sys_recv_func = NULL;

static sendto_pfn_t g_sys_sendto_func = NULL;
static recvfrom_pfn_t g_sys_recvfrom_func = NULL;

static write_pfn_t g_sys_write_func = NULL;
static read_pfn_t g_sys_read_func = NULL;

static poll_pfn_t g_sys_poll_func = NULL;

extern int cw_is_enable_hook_syscall(void);

int socket(int domain, int type, int protocol)
{
	HOOK_SYS_FUNC(socket);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_socket_func(domain, type, protocol);
	
	return g_sys_socket_func(domain, type, protocol);
}

int close(int fd)
{
	HOOK_SYS_FUNC(close);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_close_func(fd);
	
	return g_sys_close_func(fd);
}

int connect(int socket, const struct sockaddr *address, socklen_t address_len)
{
	HOOK_SYS_FUNC(connect);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_connect_func(socket, address, address_len);
	
	int ret = g_sys_connect_func(socket, address, address_len);
	if(!(ret < 0 && errno == EINPROGRESS))
		return ret;
	
	int pollret = 0;
	struct pollfd pollfd;
	
	for(int i = 0; i < 3; i++)
	{
		memset(&pollfd, 0x00, sizeof(pollfd));
		pollfd.fd = socket;
		pollfd.events = POLLOUT | POLLERR | POLLHUP;
		
		pollret = poll(&pollfd, 1, 25000);
		if(pollret == 1)
			break;
	}
	if(pollfd.revents & POLLOUT)
	{
		errno = 0;
		return 0;
	}
	
	int err = 0;
	socklen_t errlen = sizeof(err);
	getsockopt(socket, SOL_SOCKET, SO_ERROR, &err, &errlen);
	if(err) 
		errno = err;
	else
		errno = ETIMEDOUT;
	
	return ret;
}

int accept(int socket, struct sockaddr *address, socklen_t *address_len)
{
	HOOK_SYS_FUNC(accept);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_accept_func(socket, address, address_len);
	
	int timeout = 1;
	struct pollfd pollfd;
	memset(&pollfd, 0x00, sizeof(pollfd));
	pollfd.fd = socket;
	pollfd.events = POLLIN | POLLERR | POLLHUP;
	poll(&pollfd, 1, timeout);
	
	return g_sys_accept_func(socket, address, address_len);
}

ssize_t send(int socket, const void *buffer, size_t length, int flags)
{
	HOOK_SYS_FUNC(send);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_send_func(socket, buffer, length, flags);
	
	size_t wrotelen = 0;
	int timeout = 1;
	ssize_t writeret = g_sys_send_func(socket, buffer, length, flags);
	if(writeret == 0)
		return writeret;
	
	if(writeret > 0)
		wrotelen += writeret;	
	
	while(wrotelen < length)
	{
		struct pollfd pollfd;
		memset(&pollfd, 0x00, sizeof(pollfd));
		pollfd.fd = socket;
		pollfd.events = POLLOUT | POLLERR | POLLHUP;
		poll(&pollfd, 1, timeout);
		
		writeret = g_sys_send_func(socket, (const char *)buffer + wrotelen, length - wrotelen, flags);
		if(writeret <= 0)
			break;
		wrotelen += writeret;
	}
	if(writeret <= 0 && wrotelen == 0)
		return writeret;
	
	return wrotelen;
}

ssize_t recv(int socket, void *buffer, size_t length, int flags)
{
	HOOK_SYS_FUNC(recv);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_recv_func(socket, buffer, length, flags);
	
	int timeout = 1;
	struct pollfd pollfd;
	memset(&pollfd, 0x00, sizeof(pollfd));
	pollfd.fd = socket;
	pollfd.events = POLLIN | POLLERR | POLLHUP;

	poll(&pollfd, 1, timeout);

	return g_sys_recv_func(socket, buffer, length, flags);
}

ssize_t sendto(int socket, const void *message, size_t length, int flags, const struct sockaddr *dest_addr, socklen_t dest_len)
{
	HOOK_SYS_FUNC(sendto);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
	
	ssize_t ret = g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
	if(ret < 0 && EAGAIN == errno)
	{
		int timeout = 1;
		struct pollfd pollfd;
		memset(&pollfd, 0x00, sizeof(pollfd));
		pollfd.fd = socket;
		pollfd.events = POLLOUT | POLLERR | POLLHUP;
		poll(&pollfd, 1, timeout);
		
		ret = g_sys_sendto_func(socket, message, length, flags, dest_addr, dest_len);
	}
	return ret;
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags, struct sockaddr *address, socklen_t *address_len)
{
	HOOK_SYS_FUNC(recvfrom);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_recvfrom_func(socket, buffer, length, flags, address, address_len);
	
	int timeout = 1;
	struct pollfd pollfd;
	memset(&pollfd, 0x00, sizeof(pollfd));
	pollfd.fd = socket;
	pollfd.events = POLLIN | POLLERR | POLLHUP;
	poll(&pollfd, 1, timeout);
	
	ssize_t ret = g_sys_recvfrom_func(socket, buffer, length, flags, address, address_len);
	return ret;
}

ssize_t write(int fildes, const void *buf, size_t nbyte)
{
	HOOK_SYS_FUNC(write);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_write_func(fildes, buf, nbyte);
	
	size_t wrotelen = 0;
	ssize_t writeret = g_sys_write_func(fildes, (const char *)buf + wrotelen, nbyte - wrotelen);
	if(writeret == 0)
		return writeret;
	
	if(writeret > 0)
		wrotelen += writeret;
	
	while(wrotelen < nbyte)
	{
		int timeout = 1;
		struct pollfd pollfd;
		memset(&pollfd, 0x00, sizeof(pollfd));
		pollfd.fd = fildes;
		pollfd.events = POLLOUT | POLLERR | POLLHUP;
		poll(&pollfd, 1, timeout);
		
		writeret = g_sys_write_func(fildes, (const char *)buf + wrotelen, nbyte - wrotelen);
		if(writeret <= 0)
			break;
		wrotelen += writeret;
	}
	
	if(writeret <= 0 && wrotelen == 0)
		return writeret;
	
	return wrotelen;
}

ssize_t read(int fildes, void *buf, size_t nbyte)
{
	HOOK_SYS_FUNC(read);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_read_func(fildes, buf, nbyte);
	
	int timeout = 1;
	struct pollfd pollfd;
	memset(&pollfd, 0x00, sizeof(pollfd));
	pollfd.fd = fildes;
	pollfd.events = POLLIN | POLLERR | POLLHUP;
	poll(&pollfd, 1, timeout);
	
	ssize_t readret = g_sys_read_func(fildes, (char *)buf, nbyte);
	
	return readret;
}

extern int cw_poll_inner(struct pollfd fds[], nfds_t nfds, int timeout);

int poll(struct pollfd fds[], nfds_t nfds, int timeout)
{
	HOOK_SYS_FUNC(poll);
	
	if(!cw_is_enable_hook_syscall())
		return g_sys_poll_func(fds, nfds, timeout);
	
	return cw_poll_inner(fds, nfds, timeout);
}
