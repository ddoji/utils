#ifndef THREAD_H_
#define THREAD_H_

#include <pthread.h>

typedef struct thread_info {
	pthread_t		tid;
	void 			*(*thread_func)(void *);
	void        	*argv;
	pthread_mutex_t lock;
	pthread_cond_t  cond;
	int				run;
}THREAD_INFO;

int		thread_create(THREAD_INFO *pThreadInfo);
void 	thread_destroy(THREAD_INFO *pThreadInfo);
int 	thread_lock(THREAD_INFO *pThreadInfo);
void 	thread_unlock(THREAD_INFO *pThreadInfo);
int 	thread_condwait_timeout(THREAD_INFO *pThreadInfo, int milliseconds);
void 	thread_condwait(THREAD_INFO *pThreadInfo);
void 	thread_condsignal(THREAD_INFO *pThreadInfo);

#endif /* THREAD_H_ */
