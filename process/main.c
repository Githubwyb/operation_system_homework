#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/syscall.h>
#include <pthread.h>

const char *const fileName = "input.txt";
static unsigned int N = 0;
static unsigned int M = 0;

int parseFile(void)
{
    FILE *pFile = NULL;
    char *pStr = NULL;
    int rc = 0;

    do
    {
        if ((pFile = fopen(fileName, "r")) == NULL)
        {
            LOG_ERROR("Can't open file %s", fileName);
            break;
        }

        pStr = (char *)malloc(100);
        if (pStr == NULL)
        {
            LOG_ERROR("malloc error");
            break;
        }

        fgets(pStr, 100, pFile);
        rc = sscanf(pStr, "N=%d", &N);
        if (rc == 0)
        {
            LOG_ERROR("file content isn't a stantard format: %s", pStr);
            break;
        }

        fgets(pStr, 100, pFile);
        rc = sscanf(pStr, "M=%d", &M);
        if (rc == 0)
        {
            LOG_ERROR("file content isn't a stantard format: %s", pStr);
            break;
        }

        fclose(pFile);
        return 0;
    } while (0);

    fclose(pFile);
    return -1;
}

void *threadHandler(void *param)
{
    LOG_DEBUG("");
    return NULL;
}

int main(int argc, char const *argv[])
{
    pthread_t pThread[100];
    int i = 0;
    int runTime = 0;
    struct timespec timeSpecStart;
    struct timespec timeSpecEnd;

    LOG_DEBUG("Hello, gcc\n");

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeSpecStart);

    if (parseFile() != 0)
    {
        LOG_ERROR("parse file error");
        return -1;
    }
    LOG_DEBUG("parse file success N = %d, M = %d", N, M);

    // 创建线程A
    for (i = 0; i < N; i++)
    {
        if (pthread_create(&(pThread[i]), NULL, threadHandler, NULL) == -1)
        {
            LOG_ERROR("fail to create pthread %d", i);
            break;
        }
    }

    // 等待线程结束
    void *result;
    for (i = 0; i < N; i++)
    {
        if (pthread_join(pThread[i], &result) == -1)
        {
            LOG_ERROR("fail to recollect %d", i);
            continue;
        }
    }

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeSpecEnd);
    runTime = (timeSpecEnd.tv_sec - timeSpecStart.tv_sec) * 1000000000 + (timeSpecEnd.tv_nsec - timeSpecStart.tv_nsec);
    LOG_DEBUG("runtime %d ns", runTime);

    return 0;
}