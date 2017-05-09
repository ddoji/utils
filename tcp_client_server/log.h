#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>

#define LOG_LEVEL_NONE			0
#define LOG_LEVEL_ERROR         1
#define LOG_LEVEL_WARN          2
#define LOG_LEVEL_INFO          3
#define LOG_LEVEL_DEBUG         4
#define LOG_LEVEL_TRACE         5

#if 1
#define LOG_LEVEL				LOG_LEVEL_DEBUG
#else
#define LOG_LEVEL				LOG_LEVEL_NONE
#endif

#define LOG_WRAPPER(level, msg, ...) \
    do { \
    	if (level <= LOG_LEVEL) {\
			struct timeval  tp; \
			struct tm      *ctp; \
			char date_time[32] = {}; \
			if (!gettimeofday(&tp, NULL)) { \
				if ((ctp = localtime(&tp.tv_sec))) { \
					sprintf(date_time, "%02d:%02d:%02d:%03d", ctp->tm_hour, ctp->tm_min, ctp->tm_sec, (int)(tp.tv_usec/1000));  \
				} else { \
					sprintf(date_time, "%02d:%02d:%02d:%03d", 0, 0, 0, 0); \
				} \
			} else { \
				sprintf(date_time, "%02d:%02d:%02d:%03d", 0, 0, 0, 0); \
			} \
			switch(level) {\
			case LOG_LEVEL_ERROR    :printf("[%s][%u][%5.5s][%15.15s(%-4d)]: " msg "\n", date_time, (unsigned int)pthread_self(), "ERROR", __FUNCTION__, __LINE__, ##__VA_ARGS__); break;\
			case LOG_LEVEL_WARN     :printf("[%s][%u][%5.5s][%15.15s(%-4d)]: " msg "\n", date_time, (unsigned int)pthread_self(), "WARN", __FUNCTION__, __LINE__, ##__VA_ARGS__); break;\
			case LOG_LEVEL_INFO     :printf("[%s][%u][%5.5s][%15.15s(%-4d)]: " msg "\n", date_time, (unsigned int)pthread_self(), "INFO", __FUNCTION__, __LINE__, ##__VA_ARGS__); break;\
			case LOG_LEVEL_DEBUG    :printf("[%s][%u][%5.5s][%15.15s(%-4d)]: " msg "\n", date_time, (unsigned int)pthread_self(), "DEBUG", __FUNCTION__, __LINE__, ##__VA_ARGS__); break;\
			case LOG_LEVEL_TRACE    :printf("[%s][%u][%5.5s][%15.15s(%-4d)]: " msg "\n", date_time, (unsigned int)pthread_self(), "TRACE", __FUNCTION__, __LINE__, ##__VA_ARGS__); break;\
			}\
    	};\
    } while(0)

#define LOGE(msg, ...)		LOG_WRAPPER(LOG_LEVEL_ERROR,   msg, ##__VA_ARGS__)
#define LOGW(msg, ...)		LOG_WRAPPER(LOG_LEVEL_WARN,    msg, ##__VA_ARGS__)
#define LOGI(msg, ...)		LOG_WRAPPER(LOG_LEVEL_INFO,    msg, ##__VA_ARGS__)
#define LOGD(msg, ...)		LOG_WRAPPER(LOG_LEVEL_DEBUG,   msg, ##__VA_ARGS__)
#define LOGT(msg, ...)		LOG_WRAPPER(LOG_LEVEL_TRACE,   msg, ##__VA_ARGS__)

#endif /* LOG_H_ */
