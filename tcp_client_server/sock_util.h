
#ifndef SOCK_UTIL_H_
#define SOCK_UTIL_H_

#define MAX_PACKET_SIZE		1024 * 10

typedef enum {
	SOCK_TYPE_NONE		= 0,
	SOCK_TYPE_STREAM	= 1,
	SOCK_TYPE_DGRAM  	= 2
}SOCK_TYPE;

typedef struct tagSockInfo{
    int    		sock_fd;
    int		    sock_type;
    int    		sock_port;
    char   		sock_addr[32];
    //struct sockaddr_in sock_addr;
} SOCK_INFO, *PSOCK_INFO;

PSOCK_INFO sock_init();
int  sock_create_new(PSOCK_INFO *pSock, SOCK_TYPE sock_type, const char *addr, const int port);
int  sock_create(PSOCK_INFO *pSock, int socket);
void sock_close(PSOCK_INFO pSock);
void sock_free(PSOCK_INFO *pSock);

int sock_bind(PSOCK_INFO pSock);
int sock_listen(PSOCK_INFO pSock, int backlog);
int sock_accept(int socket);
int sock_connect(PSOCK_INFO pSock);
//int sock_connect_timeout(PSOCK_INFO pSock, int sec);
int sock_check(int socket);
int sock_wait_timeout(const int socket, int sec, int msec);

int sock_send(int socket, const char *buf, const int len);
int sock_recv(int socket, char *buf, int size);

int sock_sendto(int socket, PSOCK_INFO pSockTo, const char *buf, const int len);
int sock_recvfrom(int socket, PSOCK_INFO pSockFrom, char *buf, int size);

int sock_setsndbufsize(const int socket, const int nsize);
int sock_setrcvbufsize(const int socket, const int nsize);

int sock_get_local_bindport(const int socket);
int sock_get_ipaddr(const char *hostname, char *address);
int sock_get_ipaddr_local(char *address);
int sock_check_ipaddr(const char *host);

#endif /* SOCK_UTIL_H_ */
