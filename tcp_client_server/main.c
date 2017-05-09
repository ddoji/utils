#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "sock_util.h"
#include "log.h"

#pragma pack(1)
typedef struct tagResponse
{
    int   sock_fd;
    char  host[15];
    int   port;
} RESPONSE_SVC ;
#pragma pack()

void create_tcpserver(const int port)
{
	PSOCK_INFO pSockServer;
	PSOCK_INFO pSockClient = NULL;

	if (sock_create_new(&pSockServer, SOCK_TYPE_STREAM, NULL, port) != 1) {
		LOGE("tcp server - socket create fail");
		sock_free(&pSockServer);
		return;
	}

	if (sock_bind(pSockServer) != 1) {
		LOGE("tcp server - socket bind fail");
		sock_free(&pSockServer);
		return;
	}

	if (sock_listen(pSockServer, 512) != 1) {
		LOGE("tcp server - socket listen fail");
		sock_free(&pSockServer);
		return;
	}

	int rc;

	LOGI("Successfully create tcp server on [%s:%d]", pSockServer->sock_addr, pSockServer->sock_port);

	while(1) {
		if ((rc = sock_wait_timeout(pSockServer->sock_fd, 0, 100)) > 0) {
			if ((rc = sock_accept(pSockServer->sock_fd)) > 0) {
				if (sock_create(&pSockClient, rc) == 1) {
					LOGI("new connection from %s:%d", pSockClient->sock_addr, pSockClient->sock_port);
				} else {
					LOGE("couldn't create client socket info");
					sock_free(&pSockClient);
				}

			} else if (rc == 0) {
				LOGE("sock_accept error : 0");
			} else {
				LOGE("sock_accept error : %s", strerror(errno));
			}
		} else if (rc == 0) {
			//LOGD("timeout");
		} else {
			LOGE("sock_wait_timeout error : %s", strerror(errno));
		}

		if (pSockClient) {
			char buf[1024] = {};
			int size = sizeof(buf);
			if ((rc = sock_wait_timeout(pSockClient->sock_fd, 0, 100)) > 0) {
				if (!sock_check(pSockClient->sock_fd)) {
					LOGI("connection closed from %s:%d", pSockClient->sock_addr, pSockClient->sock_port);
					sock_free(&pSockClient);
				} else {
					rc = sock_recv(pSockClient->sock_fd, buf, size);
					LOGI("read result (%d)[%s]", rc, buf);
					rc = sock_send(pSockClient->sock_fd, "hello kuno12", 12);
					LOGI("send result (%d)", rc);
				}
				//read data
			} else if (rc == 0) {
				LOGD("timeout");
			} else {
				LOGE("sock_wait_timeout for client error : %s", strerror(errno));
				sock_free(&pSockClient);
			}
		}
	}

	sock_free(&pSockServer);
}

void create_tcpclient(int port)
{
	PSOCK_INFO pSockClient;
	char buf[1024] = {};
	int size = sizeof(buf);

	if (sock_create_new(&pSockClient, SOCK_TYPE_STREAM, "127.0.0.1", port) != 1) {
		LOGE("tcp client - socket create fail");
		sock_free(&pSockClient);
		return;
	}

	if (sock_connect(pSockClient) == 1) {
	//if (sock_connect_timeout(pSockClient, 3) == 1) {
		LOGI("connection success to [%s:%d][%d]", pSockClient->sock_addr, pSockClient->sock_port
				, sock_get_local_bindport(pSockClient->sock_fd));
	} else {
		LOGE("connection fail");
	}
	int count = 5;
	while(count -- > 0) {
		int rc = sock_send(pSockClient->sock_fd, "hello kuno", 10);
		LOGI("send result : %d", rc);
		rc = sock_recv(pSockClient->sock_fd, buf, size);
		LOGI("recv result : %d [%s]", rc, buf);
		usleep(200000);
	}
	LOGI("connection close request");

	sock_free(&pSockClient);
}

void create_udpserver(const int port)
{
	PSOCK_INFO pSock, pSockClient;
	char buf[1024] = {};
	int size = sizeof(buf);
	int rc;

	if (sock_create_new(&pSock, SOCK_TYPE_DGRAM, NULL, port) != 1) {
		LOGE("udp server - socket create fail");
		sock_free(&pSock);
		return;
	}

	if (sock_bind(pSock) != 1) {
		LOGE("udp server - socket bind fail");
		sock_free(&pSock);
		return;
	}

	pSockClient = sock_init();
	while(1) {
		memset(buf, 0x00, size);
		if ((rc = sock_wait_timeout(pSock->sock_fd, 0, 500)) > 0) {
			rc = sock_recvfrom(pSock->sock_fd, pSockClient, buf, size);
			if (rc > 0) {
				LOGI("recv from [%s:%d], data[%s]", pSockClient->sock_addr, pSockClient->sock_port, buf);


				rc = sock_sendto(pSock->sock_fd, pSockClient, "hello kuno", 10);
				if (rc > 0) {
					LOGI("send to [%s:%d] success", pSockClient->sock_addr, pSockClient->sock_port);
				}
				memset(pSockClient->sock_addr, 0x00, sizeof(pSockClient->sock_addr));
				pSockClient->sock_port = -1;
			} else {
				break;
			}
		} else if (rc == 0) {
			LOGD("timeout");
		} else {
			LOGE("sock_wait_timeout error : %s", strerror(errno));
		}
	}

	sock_free(&pSockClient);
	sock_free(&pSock);

}

void create_udpclient(const int port)
{
	PSOCK_INFO pSock;
	char buf[1024] = {};
	int size = sizeof(buf);
	int rc;

	if (sock_create_new(&pSock, SOCK_TYPE_DGRAM, "localhost", port) != 1) {
		LOGE("udp client - socket create fail");
		sock_free(&pSock);
		return;
	}

	rc = sock_sendto(pSock->sock_fd, pSock, "hello kuno123", 13);
	LOGI("send result : %d", rc);
	rc = sock_recvfrom(pSock->sock_fd, NULL, buf, size);
	LOGI("recv result : %d, [%s]", rc, buf);

	sock_free(&pSock);
}

int main(int argc, char *argv[]){


	int mode = atoi(argv[1]);
	switch(mode) {
	case 1:create_tcpserver(5000);break;
	case 2:create_tcpclient(5000); break;
	case 3:create_udpserver(5001);break;
	case 4:create_udpclient(5001);break;
	}

	return 0;
}

