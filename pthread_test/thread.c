#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

#include "thread.h"
#include "log.h"

int thread_create(THREAD_INFO *pThreadInfo) {
	int rc;
	int retry = 5;

	if (NULL == pThreadInfo) {
		return -1;
	}

	pthread_mutex_init(&pThreadInfo->lock, NULL);
	pthread_cond_init(&pThreadInfo->cond, NULL);

	errno = 0;
	while(1)
	{
		if ((rc = pthread_create(&pThreadInfo->tid, NULL, pThreadInfo->thread_func, (void *)pThreadInfo)) == 0) {
			LOG_INFO("thread create success with %u", (unsigned int)pThreadInfo->tid);
			pThreadInfo->run = 1;

			return 0;
		} else if (errno == EINTR || errno == EAGAIN) {
			usleep(500);
			if (retry -- < 0) break;
			continue;
		} else {
			break;
		}
	}

	LOG_ERROR("thread create fail with [%s]", strerror(errno));
	return -1;
}

void thread_destroy(THREAD_INFO *pThreadInfo) {
	int res;
	int status;

	if (NULL == pThreadInfo) {
		return;
	}

	pThreadInfo->run = 0;

	if ((res = pthread_join(pThreadInfo->tid, (void *)&status)) == 0) {
		LOG_INFO("thread terminated success with %d", status);
	} else {
		LOG_ERROR("thread [%u] terminate fail", (unsigned int)pThreadInfo->tid);
	}

	if (thread_lock(pThreadInfo)) {
		LOG_DEBUG("lock success");
		thread_unlock(pThreadInfo);
		pthread_mutex_destroy(&pThreadInfo->lock);
	}
	pthread_cond_destroy(&pThreadInfo->cond);
}

/*
 * return 1 if successful
 */
int thread_lock(THREAD_INFO *pThreadInfo) {
	int retry = 5;
	int ret = 0;
	do {
		ret = pthread_mutex_trylock(&pThreadInfo->lock);
	} while(ret == EBUSY && retry-- > 0);

	return !ret;
}

void thread_unlock(THREAD_INFO *pThreadInfo) {
	pthread_mutex_unlock(&pThreadInfo->lock);
}

int thread_condwait_timeout(THREAD_INFO *pThreadInfo, int milliseconds) {
	struct timespec   ts;
	struct timeval    tp;
	int rc;
	long nsec;
	int sec = 0;

	rc = gettimeofday(&tp, NULL);
	nsec = (tp.tv_usec + (milliseconds % 1000) * 1000UL) * 1000UL;

	// nsec range : [0 .. 999999999]
	if (nsec > 1000000000UL) {
		sec = (int)(nsec / 1000000000UL);
		nsec -= (sec * 1000000000UL);
	}
	sec += tp.tv_sec + (long)(milliseconds / 1000);

	ts.tv_sec  = sec;
	ts.tv_nsec = nsec;

	rc = pthread_cond_timedwait(&pThreadInfo->cond, &pThreadInfo->lock, &ts);
	if (rc == 0) return 0;
	else if (rc == ETIMEDOUT) return 1;

	return -rc;
}

void thread_condwait(THREAD_INFO *pThreadInfo) {
	pthread_cond_wait(&pThreadInfo->cond, &pThreadInfo->lock);
}

void thread_condsignal(THREAD_INFO *pThreadInfo) {
	pthread_cond_signal(&pThreadInfo->cond);
}
