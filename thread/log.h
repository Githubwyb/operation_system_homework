#include <stdio.h>

#define DEBUG

#ifdef DEBUG
char *splitFileName(char *str);

#define LOG_INFO(fmt, ...)                                                            \
    printf("[%lu][I][%s:%d %s] " fmt, pthread_self(), splitFileName(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    printf("\r\n");
#define LOG_DEBUG(fmt, ...)                                                           \
    printf("[%lu][D][%s:%d %s] " fmt, pthread_self(), splitFileName(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    printf("\r\n")
#define LOG_ERROR(fmt, ...)                                                           \
    printf("[%lu][E][%s:%d %s] " fmt, pthread_self(), splitFileName(__FILE__), __LINE__, __FUNCTION__, ##__VA_ARGS__); \
    printf("\r\n")

#else

#define LOG_INFO(fmt, ...)
#define LOG_DEBUG(fmt, ...)
#define LOG_ERROR(fmt, ...)

#endif
