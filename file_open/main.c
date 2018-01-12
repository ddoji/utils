#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <pthread.h>
#include <string.h>

#include <glib.h>

#include "string_util.h"

#define MAX_LINE	512
//test
typedef struct tagLogInfo {
	char date_time[32];
	long tid;
	char level[6];
	char func[32];
	int  line;
	char msg[128];
} LOG_INFO;



int main(int argc, char *argv[]){
	FILE *fp = NULL;
	char buf[MAX_LINE] = {};
	char *token;
	char *str;

	const char *filename = "sample.txt";

	if (access(filename, F_OK) != 0)
	{
		fprintf(stderr, "file not exist\n");
		return -1;
	}

	if ((fp = fopen(filename, "rt")) == NULL) {
		fprintf(stderr, "file open error\n");
		return -1;
	}

	while(fgets(buf, MAX_LINE, fp) != NULL) {
		//printf("str:%s\n", str);
		buf[strcspn(buf, "\n")] = 0;

		token = strtok(buf, "[]()");
		while(token != NULL) {
			str = trim(token);
			printf("^%s^ ", str);
			token = strtok(NULL, "[]()");
		}
		printf("\n");
	}

	fclose(fp);

	if ((fp = popen("ls -al", "r")) == NULL) {
		fprintf(stderr, "popen error\n");
		return -1;
	}

	while(fgets(buf, MAX_LINE, fp) != NULL) {
		printf("%s", buf);
	}
	pclose(fp);

	char uri[] = "http://a.b.c:80/abc.htm";
	char protocol[20] = {};
	char host[100] = {};
	char path[100] = {};
	if(sscanf(uri,"%20[^:/]://%100[^/]/%s", protocol, host, path) == 3) {
		printf ("protocol=<%s>, host=<%s>, path=<%s>\n", protocol, host, path);
	}

	char sentense[] = "[13:30:51:417][536969248][ INFO][thread_cre(25  )]: thread create success with 537269208\n";
	LOG_INFO logInfo = {};

	int rc = sscanf(sentense, "[%[^]]][%*[^]]][%[^]]][%[^(](%d%*[^:]: %[^\n]s", logInfo.date_time, logInfo.level, logInfo.func, &logInfo.line, logInfo.msg);
	printf("rc:%d, date_time:%s, level:%s, func:%s, line:%d, msg:%s\n", rc, logInfo.date_time, trim(logInfo.level), logInfo.func, logInfo.line, logInfo.msg);

	//%[123]s : 12345 -> 123
	//%[^123]s : 12345 -> 45
	//%[0-9]s  : 123abc -> 123
	//%[a-zA-Z]s : 영문자만

	return 0;
}

