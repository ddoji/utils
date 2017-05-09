#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "thread.h"
#include "Log.h"

int val = 0;

void *thread_func(void *arg) {
	THREAD_INFO *pThreadInfo = (THREAD_INFO *)arg;
	int rc;

	LOG_DEBUG("thread argument : %s", (char *)pThreadInfo->argv);

	while(pThreadInfo->run) {
		if (thread_lock(pThreadInfo)) {

			LOG_DEBUG("cond wait timeout)");
			if ((rc = thread_condwait_timeout(pThreadInfo, 300)) == 0) {
				LOG_DEBUG("do loop : %d", val);
			} else if (rc > 0 ) {
				LOG_DEBUG("timeout");
			} else {
				LOG_ERROR("thread condwait timeout error : %s", strerror(-rc));
			}

			thread_unlock(pThreadInfo);
		} else {
			usleep(100);
		}
	}

	LOG_INFO("thread end");

	return (void *)0;
}

int main(int argc, char *argv[]){
	THREAD_INFO threadInfo = {};
	int loop = 10;
	threadInfo.thread_func = thread_func;
	threadInfo.argv = (void *)"hello kuno";

	if (thread_create(&threadInfo) == 0) {
		while(loop-- > 0) {
			if (thread_lock(&threadInfo)) {
				val++;
				LOG_DEBUG("send signal");
				thread_condsignal(&threadInfo);
				thread_unlock(&threadInfo);
			}
			usleep(1000000);
		}
		thread_destroy(&threadInfo);
	}

	return 0;
}

