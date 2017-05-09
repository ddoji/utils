#include "sock_util.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/resource.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <poll.h>
#include <errno.h>

#include "log.h"

#define VERIFY(...) VERIFY_(__VA_ARGS__, _1, _0)(__VA_ARGS__)
#define VERIFY_(_0, _1, X, ...) VERIFY ## X
#define VERIFY_0(expr) do if (!(expr)) { return; } while(0)
#define VERIFY_1(expr, ret) do if (!(expr)) { return ret; } while(0)

/******************************************************************************
 * @brief
 * 		Socket 구조체를 생성하고 초기화 한다.
 * @param
 *
 * @return
 * 		PSOCK_INFO : 초기화된 Socket 구조체 포인터
 ******************************************************************************/
PSOCK_INFO sock_init()
{
	PSOCK_INFO  pSock = 0x00;

	pSock = (PSOCK_INFO)malloc(sizeof(SOCK_INFO));

	VERIFY(pSock != NULL, NULL);

    pSock->sock_fd        = -1;
    pSock->sock_type      = SOCK_TYPE_NONE;
    pSock->sock_port      = 0;
    memset(pSock->sock_addr, 0x00, sizeof(pSock->sock_addr));

    return pSock;
}

/******************************************************************************
 * @brief
 * 		지정된 Socket Type 및 종류의 Socket을 생성한다.
 * @param
 *		PSOCK_INFO : 할당된 소켓 정보 구조체
 * @return
 *		1 if successful, otherwise < 0
 ******************************************************************************/
int sock_create_new(PSOCK_INFO *pSock, SOCK_TYPE sock_type, const char *addr, const int port)
{
	int fd;
	int type;

	VERIFY(port >= 0 && port <= 65535, -1);

	*pSock = sock_init();
	VERIFY(*pSock != NULL, -1);

	switch(sock_type) {
	case SOCK_TYPE_STREAM:	type = SOCK_STREAM; break;
	case SOCK_TYPE_DGRAM:	type = SOCK_DGRAM; break;
	default:
		return -1;
	}
	if ((fd = socket(AF_INET, type, 0)) < 0) {
		LOGE("Couldn't create socket, reason : %s", strerror(errno));
		return -2;
	}
	(*pSock)->sock_fd = fd;
	(*pSock)->sock_type = type;
	(*pSock)->sock_port = port;
	if (addr) {
		strcpy((*pSock)->sock_addr, addr);
	}

	return 1;
}

/******************************************************************************
 * @brief
 * 		socket descriptor로부터 소켓 정보를 생성한다.
 * @param
 *		PSOCK_INFO : 할당된 소켓 정보 구조체
 * @return
 *		1 if successful, otherwise < 0
 ******************************************************************************/
int sock_create(PSOCK_INFO *pSock, int socket)
{
	int len;
	int on = 1;
	struct sockaddr_in addr;

	VERIFY(socket >= 0, -1);

	*pSock = sock_init();

	VERIFY(*pSock != NULL, -1);

	len = sizeof(addr);
	if (getpeername(socket, (struct sockaddr *)&addr, (socklen_t *)&len) != 0) {
		LOGE("Couldn't get peer information from [fd:%d]", socket);
		return -2;
	}

	strcpy((*pSock)->sock_addr, inet_ntoa(addr.sin_addr));
	(*pSock)->sock_port = ntohs(addr.sin_port);
	(*pSock)->sock_fd = socket;

	len = sizeof(int);
	getsockopt(socket, SOL_SOCKET, SO_TYPE, &(*pSock)->sock_type, (socklen_t *)&len);

	sock_setsndbufsize((*pSock)->sock_fd, MAX_PACKET_SIZE);
	sock_setrcvbufsize((*pSock)->sock_fd, MAX_PACKET_SIZE);

	setsockopt((*pSock)->sock_fd, SOL_SOCKET, SO_KEEPALIVE,(char *)&on,sizeof(on));
	// Turn off Nagle's algorithm
	setsockopt((*pSock)->sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

	ioctl((*pSock)->sock_fd, FIONBIO, (char *)&on);

	return 1;
}

/******************************************************************************
 * @brief
 * 		disconnect from socket.
 * @param
 *		pSock : 할당된 소켓 정보 구조체
 * @return
 *
 ******************************************************************************/
void sock_close(PSOCK_INFO pSock)
{
	VERIFY(pSock != NULL);
	VERIFY(pSock->sock_fd >= 0);

	if (pSock->sock_type == SOCK_STREAM) {
		shutdown(pSock->sock_fd, 2);
	}

	close(pSock->sock_fd);
	pSock->sock_fd = -1;
}

/******************************************************************************
 * @brief
 * 		Socket 정보 및 할당된 구조체를 삭제한다(만약 연결이 된 상태라면 close를 수행한다).
 * @param
 *		PSOCK_INFO : 할당된 소켓 정보 구조체
 * @return
 *
 ******************************************************************************/
void sock_free(PSOCK_INFO *pSock)
{
	VERIFY(pSock != NULL);

	sock_close(*pSock);

	free(*pSock);
	*pSock = 0x00;
}

/******************************************************************************
 * @brief
 * 		bind socket.
 * @param
 *		pSock : 할당된 소켓 정보 구조체
 * @return
 * 		int : 1 if successful, otherwise < 0
 ******************************************************************************/
int sock_bind(PSOCK_INFO pSock)
{
    int  on = 1;
    struct sockaddr_in serveraddr;

    VERIFY(pSock != NULL, 0);
	VERIFY(pSock->sock_fd >= 0, 0);

    setsockopt(pSock->sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
#ifdef SO_REUSEPORT
    on = 1;
    setsockopt(pSock->sock_fd, SOL_SOCKET, SO_REUSEPORT, &on, sizeof(on));
#endif

    memset(&serveraddr, 0x00, sizeof(struct sockaddr_in));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port   = htons(pSock->sock_port);

    if (*pSock->sock_addr == 0x00) {
    	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    	strcpy(pSock->sock_addr, inet_ntoa(serveraddr.sin_addr));
    } else {
    	serveraddr.sin_addr.s_addr = inet_addr(pSock->sock_addr);
    	if (serveraddr.sin_addr.s_addr == INADDR_NONE) {
    		/* Get IP address if bindAddress is a hostname */
    		char addr[50] = {0,};
    		if (!sock_get_ipaddr(pSock->sock_addr, addr)) {
    			LOGE("Couldn't get ip address from hostname [%s]", pSock->sock_addr);
    			return -1;
    		}

    		serveraddr.sin_addr.s_addr = inet_addr(addr);
    		memset(pSock->sock_addr, 0x00, sizeof(pSock->sock_addr));
    		strcpy(pSock->sock_addr, addr);
    	}
    }

    if (bind(pSock->sock_fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    	LOGE("Couldn't bind to [%s:%d], reason:%s", inet_ntoa(serveraddr.sin_addr), ntohs(serveraddr.sin_port), strerror(errno));
    	return -2;
    }

    if (pSock->sock_type == SOCK_STREAM) {
    	setsockopt(pSock->sock_fd, SOL_SOCKET, SO_KEEPALIVE,(char *)&on,sizeof(on));
    }

    ioctl(pSock->sock_fd, FIONBIO, (char *)&on);

    return 1;
}

/******************************************************************************
 * @brief
 * 		listen socket.
 * @param
 *		pSock : 할당된 소켓 정보 구조체
 * @return
 * 		int : 1 if successful, otherwise  0
 ******************************************************************************/
int sock_listen(PSOCK_INFO pSock, int backlog)
{
	int backlog_;
    VERIFY(pSock != NULL, 0);
	VERIFY(pSock->sock_fd >= 0, 0);

	backlog_ = backlog;
    if (backlog_ < 0) backlog_ = 0;

    listen(pSock->sock_fd, backlog_);

    return 1;
}

/******************************************************************************
 * @brief
 * 		accept socket.
 * @param
 *		pSock : 할당된 소켓 정보 구조체
 * @return
 * 		int : 0 > if successful, otherwise <=  0
 ******************************************************************************/
int sock_accept(int socket)
{
	int                 clilen, newfd;
	struct sockaddr_in  cliaddr;

	VERIFY(socket >= 0, -1);

	clilen = sizeof(cliaddr);
	memset(&cliaddr, 0x00, clilen);

	newfd = accept(socket, (struct sockaddr *)&cliaddr, (socklen_t*)&clilen);

	if (newfd < 0) {
		if (errno == EAGAIN || errno == ECONNABORTED || /*errno == EINTR ||*/
				errno == EWOULDBLOCK) {
			return 0;
		} else {
		}
	}
	return newfd;
}

/******************************************************************************
 * @brief
 * 		connect to server.
 * @param
 *		pSock : 할당된 소켓 정보 구조체
 * @return
 * 		int : 1 if successful, otherwise <  0
 ******************************************************************************/
int sock_connect(PSOCK_INFO pSock)
{
	int rc;
	int on = 1;
    struct sockaddr_in remoteaddr;

    VERIFY(pSock != NULL, -1);
    VERIFY(pSock->sock_fd >= 0, -1);
    VERIFY(*pSock->sock_addr != 0x00, -1);

    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port   = htons(pSock->sock_port);
    remoteaddr.sin_addr.s_addr = inet_addr(pSock->sock_addr);
    if (remoteaddr.sin_addr.s_addr == INADDR_NONE) {
    	/* Get IP address if bindAddress is a hostname */
    	char addr[50] = {0,};
    	if (!sock_get_ipaddr(pSock->sock_addr, addr)) {
    		LOGE("Couldn't get ip address from hostname [%s]", pSock->sock_addr);
    		return -2;
    	}

    	remoteaddr.sin_addr.s_addr = inet_addr(addr);
    	memset(pSock->sock_addr, 0x00, sizeof(pSock->sock_addr));
    	strcpy(pSock->sock_addr, addr);
    }

    rc = connect(pSock->sock_fd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr));
    if (rc < 0) {
    	LOGE("couldn't connect to server [%s:%d], reason:%s", pSock->sock_addr, pSock->sock_port, strerror(errno));
    	return -3;
    }

	sock_setsndbufsize(pSock->sock_fd, MAX_PACKET_SIZE);
	sock_setrcvbufsize(pSock->sock_fd, MAX_PACKET_SIZE);

	setsockopt(pSock->sock_fd, SOL_SOCKET, SO_KEEPALIVE,(char *)&on,sizeof(on));
	// Turn off Nagle's algorithm
	setsockopt(pSock->sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));

	return 1;
}

/******************************************************************************
 * @brief
 * 		connect to server.
 * @param
 *		pSock : 할당된 소켓 정보 구조체
 * @return
 * 		int : 1 if successful, otherwise <  0
 ******************************************************************************/
//int sock_connect_timeout(PSOCK_INFO pSock, int sec)
//{
//    int     rc = 0;
//    int     on = 1;
//    int     error = 0;
//    int     len;
//    struct sockaddr_in remoteaddr;
//
//
//    VERIFY(pSock != NULL, -1);
//	VERIFY(pSock->sock_fd >= 0, -1);
//	VERIFY(*pSock->sock_addr != 0x00, -1);
//
//	ioctl(pSock->sock_fd, FIONBIO, (char *)&on);
//
//    remoteaddr.sin_family = AF_INET;
//    remoteaddr.sin_port   = htons(pSock->sock_port);
//    remoteaddr.sin_addr.s_addr = inet_addr(pSock->sock_addr);
//    if (remoteaddr.sin_addr.s_addr == INADDR_NONE) {
//    	/* Get IP address if bindAddress is a hostname */
//    	char addr[50] = {0,};
//    	if (!sock_get_ipaddr(pSock->sock_addr, addr)) {
//    		LOGE("Couldn't get ip address from hostname [%s]", pSock->sock_addr);
//    		return -2;
//    	}
//
//    	remoteaddr.sin_addr.s_addr = inet_addr(addr);
//    	memset(pSock->sock_addr, 0x00, sizeof(pSock->sock_addr));
//    	strcpy(pSock->sock_addr, addr);
//    }
//
//    rc = connect(pSock->sock_fd, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr));
//    if (rc < 0) {
//    	if (errno != EINPROGRESS) {
//    		LOGE("couldn't connect to server [%s:%d], reason:%s", pSock->sock_addr, pSock->sock_port, strerror(errno));
//    		return -3;
//    	} else {
//    		struct  timeval tv;
//    		fd_set  fdrset, fdwset;
//
//    		FD_ZERO (&fdrset);
//    		FD_SET(pSock->sock_fd, &fdrset);
//    		fdwset = fdrset;
//
//    		tv.tv_sec  = sec;
//    		tv.tv_usec = 0;
//
//    		LOGD("1");
//    		if ((rc = select(pSock->sock_fd + 1, &fdrset, &fdwset, 0, &tv)) <= 0)
//    		{
//    			LOGE("couldn't connect to server [%s:%d], reason:select fail", pSock->sock_addr, pSock->sock_port);
//    			return -3;
//    		}
//    		LOGD("2");
//    		errno = 0;
//    		if (FD_ISSET(pSock->sock_fd, &fdrset) || FD_ISSET(pSock->sock_fd, &fdwset)) {
//    		    len = sizeof(error);
//    		    if (getsockopt(pSock->sock_fd, SOL_SOCKET, SO_ERROR, (char *)&error, (socklen_t *)&len) < 0) {
//    		    	LOGE("couldn't get socket error state, reason:%s", strerror(errno));
//    		    	return -4;
//    		    }
//    		    LOGD("3 : %d", error);
//    		} else {
//    			LOGE("couldn't connect to server [%s:%d], reason:select fail", pSock->sock_addr, pSock->sock_port);
//    			return -4;
//    		}
//    	}
//    }
//
//    if (error) {
//    	LOGE("couldn't connect to server [%s:%d], reason:%s", pSock->sock_addr, pSock->sock_port, strerror(error));
//    	return -3;
//    }
//    LOGD("4");
//	sock_setsndbufsize(pSock->sock_fd, MAX_PACKET_SIZE);
//	sock_setrcvbufsize(pSock->sock_fd, MAX_PACKET_SIZE);
//
//	setsockopt(pSock->sock_fd, SOL_SOCKET, SO_KEEPALIVE,(char *)&on,sizeof(on));
//	// Turn off Nagle's algorithm
//	setsockopt(pSock->sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&on, sizeof(on));
//
//    return 1;
//}

int sock_check(int socket)
{
	int count = 0;
	int rc;

	if ((rc = ioctl(socket, FIONREAD, &count)) < 0) {
		return 0;
	}
	return count;
}

/******************************************************************************
 * @brief
 * 		socket descriptor 이벤트 감지
 * @param
 *  	socket : socket descriptor
 *		nsize : size
 * @return
 * 		int : 1 success, 0 timeout, otherwise error
 ******************************************************************************/
int sock_wait_timeout(const int socket, int sec, int msec)
{
//	RLIMIT_NOFILE;
//	struct rlimit rlim;
//
//	getrlimit(RLIMIT_NOFILE, &rlim);
//	printf("Open file %d : %d\n", rlim.rlim_cur, rlim.rlim_max);

    struct pollfd   fds;
    int             timeout;
    int             rc;

    VERIFY(socket > 0, 0);

    memset(&fds, 0 , sizeof(struct pollfd));

    fds.fd = socket;
    fds.events = POLLIN | POLLERR | POLLHUP | POLLNVAL;

    timeout = sec * 1000 + msec;
    if (timeout <= 0) timeout = 10;     // default 10 ms

    rc = poll((struct pollfd*)&fds, 1, timeout);

    if (rc > 0) {
        if (fds.revents & POLLIN) {
            return 1;
        } else if ((fds.revents & POLLERR) || (fds.revents & POLLHUP) || (fds.revents & POLLNVAL)) {
            return -1;
        } else {
            return 0;
        }
    }

    return rc;
}

int sock_send(int socket, const char *buf, const int len)
{
	int rc;
	int nwritten;

	struct pollfd fds;
	int timeout;

	nwritten = 0;

	while (len - nwritten > 0) {
		rc = write(socket, buf + nwritten, len - nwritten);
		if (rc <= 0) {
			if (rc < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
				memset(&fds, 0 , sizeof(struct pollfd));

				fds.fd = socket;
				fds.events = POLLOUT;

				timeout = 3000;

				rc = poll((struct pollfd*)&fds, 1, timeout);

				if (rc > 0) {
					if (fds.revents & POLLOUT) {
						continue;
					} else {
						return nwritten;
					}
				} else {
					return nwritten;
				}

			} else if (rc == 0) {
				continue;
			} else {
				return nwritten;
			}
		}
		nwritten += rc;
	}

	return nwritten;
}

int sock_recv(int socket, char *buf, int size)
{
	int rc;
    rc = read(socket, buf, size);

    if (rc < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            rc =  0;
        } else {
            return -1;
        }
    }

    return rc;
}

int sock_sendto(int socket, PSOCK_INFO pSockTo, const char *buf, const int len)
{
	int sent;
	int rc;
	struct sockaddr_in  remoteaddr;

	VERIFY(pSockTo->sock_addr != NULL, -1);
	VERIFY(pSockTo->sock_port >= 0 && pSockTo->sock_port <= 65535, -1);
	VERIFY(buf != NULL, -1);

    remoteaddr.sin_family = AF_INET;
    remoteaddr.sin_port   = htons(pSockTo->sock_port);
    remoteaddr.sin_addr.s_addr = inet_addr(pSockTo->sock_addr);
    if (remoteaddr.sin_addr.s_addr == INADDR_NONE) {
    	/* Get IP address if bindAddress is a hostname */
    	char addr[50] = {0,};
    	if (!sock_get_ipaddr(pSockTo->sock_addr, addr)) {
    		LOGE("Couldn't get ip address from hostname [%s]", pSockTo->sock_addr);
    		return -2;
    	}

    	remoteaddr.sin_addr.s_addr = inet_addr(addr);
    }

    sent = 0;
    do {
        rc = sendto(socket, buf + sent, len - sent, 0, (struct sockaddr *)&remoteaddr, sizeof(remoteaddr));
        if (rc < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == ENOBUFS) {
            	LOGE("send data to [%s:%d] warn, reason:%s", pSockTo->sock_addr, pSockTo->sock_port, strerror(errno));
                rc = 0;
                continue;
            } else {
            	LOGE("send data to [%s:%d] error, reason:%s", pSockTo->sock_addr, pSockTo->sock_port, strerror(errno));
                sent = -1;
                break;
            }
        } else {
            sent += rc;
        }

    } while(rc > 0 && len - sent > 0);

    return sent;
}

int sock_recvfrom(int socket, PSOCK_INFO pSockFrom, char *buf, int size)
{
	int rc;
	int addrlen;
	struct sockaddr_in  remoteaddr;

	addrlen = sizeof(struct sockaddr);

	do {
		rc = recvfrom(socket, buf, size, 0, (struct sockaddr *)&remoteaddr, &addrlen);
		if (rc < 0) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				continue;
			} else {
				return -1;
			}
		} else {
			if (pSockFrom) {
				strcpy(pSockFrom->sock_addr, inet_ntoa(remoteaddr.sin_addr));
				pSockFrom->sock_port = ntohs(remoteaddr.sin_port);
			}
			break;
		}
	} while(1);

	return rc;
}

/******************************************************************************
 * @brief
 * 		set socket send buffer size
 * @param
 *  	socket : socket descriptor
 *		nsize : size
 * @return
 * 		int : 1 if successful, otherwise 0
 ******************************************************************************/
int sock_setsndbufsize(const int socket, const int nsize)
{
	int  blen = nsize;

	return !setsockopt(socket, SOL_SOCKET, SO_SNDBUF, (char *)&blen, sizeof(blen));
}

/******************************************************************************
 * @brief
 * 		set socket receive buffer size
 * @param
 *  	socket : socket descriptor
 *		nsize : size
 * @return
 * 		int : 1 if successful, otherwise 0
 ******************************************************************************/
int sock_setrcvbufsize(const int socket, const int nsize)
{
	int  blen = nsize;

	return !setsockopt(socket, SOL_SOCKET, SO_RCVBUF, (char *)&blen, sizeof(blen));
}

/******************************************************************************
 * @brief
 * 		get local bind port from OS allocate random port
 * @param
 *  	socket : socket descriptor
 * @return
 * 		int : 0 > if successful, otherwise 0
 ******************************************************************************/
int sock_get_local_bindport(const int socket)
{
	struct sockaddr_in      temp;
	int                     addrlen = sizeof(temp);

	VERIFY(socket > 0, 0);

	if (getsockname(socket, (struct sockaddr*)&temp, &addrlen) < 0) {
		return 0;
	}

	return ntohs(temp.sin_port);
}

/******************************************************************************
 * @brief
 * 		hostname으로부터 ip address를 얻는다
 * @param
 *  	hostname : hostname
 *		address : ipaddress를 저장할 포인터
 * @return
 * 		int : 1 if successful, otherwise 0
 ******************************************************************************/
int sock_get_ipaddr(const char *hostname, char *address)
{
	struct addrinfo *		l_addrInfo = NULL, *l_addrInfoCurrent = NULL;
	struct addrinfo			hints;
	struct sockaddr_in *	ipv4;
	char *					ip;

	VERIFY(hostname != NULL, 0);
	VERIFY(address != NULL, 0);

	memset( &hints, 0, sizeof(hints) );
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(hostname, NULL, &hints, &l_addrInfo) != 0) {
		return 0;
	}

	for(l_addrInfoCurrent = l_addrInfo; l_addrInfoCurrent != NULL; l_addrInfoCurrent = l_addrInfoCurrent->ai_next) {
		if (l_addrInfoCurrent->ai_family == AF_INET) {
			ipv4 = (struct sockaddr_in *)l_addrInfoCurrent->ai_addr;
			ip = inet_ntoa(ipv4->sin_addr);

			memcpy(address, ip, strlen(ip));
			break;
		}
	}

	if (l_addrInfo) {
		freeaddrinfo(l_addrInfo);
	}

	return 1;
}

/******************************************************************************
 * @brief
 * 		local ip address를 얻는다
 * @param
 *		address : ipaddress를 저장할 포인터
 * @return
 * 		int : 1 if successful, otherwise 0
 ******************************************************************************/
int sock_get_ipaddr_local(char *address)
{
	VERIFY(address != NULL, 0);

	char hostname[256] = {0,};
	if ( gethostname(hostname, 256) == 0) {
		return sock_get_ipaddr(hostname, address);
	}
	return 0;
}

/******************************************************************************
 * @brief
 * 		host가 ipaddress인지 아닌지를 체크한다.
 * @param
 *		host : hostname
 * @return
 * 		int : 1 if successful, otherwise 0
 ******************************************************************************/
int sock_check_ipaddr(const char *host)
{
	VERIFY(host != NULL, 0);

	unsigned long ulAddr  = inet_addr(host);
	if (ulAddr !=INADDR_NONE && ulAddr != INADDR_ANY) {
		struct in_addr antelope;
		antelope.s_addr = ulAddr;
		if (strcmp(inet_ntoa(antelope), host)==0) {
			return 1;
		}
	}
	return 0;
}

