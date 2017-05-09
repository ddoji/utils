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

#include <glib.h>

void __attribute__((constructor)) console_setting_for_eclipse_debugging( void ){
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
}

void thread_func(gpointer data, gpointer user_data) {

	int id = GPOINTER_TO_UINT(data);
	GThread *gThread = g_thread_self();
	printf("thread[%p] id : %d\n", gThread, id);

	usleep((id % 5) * 1000000);

	printf("thread[%p] done\n", gThread);
}

void thread_pool_create() {
	GThreadPool *pThreadPool;
	int i;

	pThreadPool = g_thread_pool_new((GFunc)thread_func, NULL, 20, TRUE, NULL);
	if (NULL == pThreadPool) {
		printf("thread pool create fail\n");
		return;
	}

	for (i = 0; i < 10; i++)
	{

		int res = g_thread_pool_push (pThreadPool, GUINT_TO_POINTER (i + 1), NULL);
		printf("push data [%d].....[res:%d]\n", i + 1, res);
	}

	int t_max_num =  g_thread_pool_get_max_threads(pThreadPool);

	printf("thread pool max: %d\n", t_max_num);

	printf("push data done.\n");
	g_thread_pool_free(pThreadPool, FALSE, TRUE);
}

gpointer thread_func1(gpointer data) {
	int id = GPOINTER_TO_UINT(data);
	GThread *gThread = g_thread_self();
	printf("thread[%p] id : %d\n", gThread, id);

	usleep((id % 5) * 1000000);

	printf("thread[%p] done\n", gThread);

	return GINT_TO_POINTER (0);;
}

void thread_create() {
	GThread *thread = g_thread_new("test", (GThreadFunc)thread_func1, GUINT_TO_POINTER (100));
	if (NULL == thread) {
		fprintf(stderr, "create thread fail\n");
		return;
	}
	printf("wait to terminated thread\n");
	gpointer ret = g_thread_join(thread);
	printf("terminated thread with %d\n", GPOINTER_TO_INT(ret));
}

int main(int argc, char *argv[]){
	thread_create();

	thread_pool_create();

	return 0;
}

