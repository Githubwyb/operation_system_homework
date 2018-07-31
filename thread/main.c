#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/syscall.h>
#include <pthread.h>

const char *const inputFileName = "input.txt";
const char *const outputFileName = "output.txt";
static unsigned int N = 0;
static unsigned int M = 0;
static unsigned int pulsNumber = 0;
static unsigned long int plusResult = 0;

int parseFile(void)
{
    FILE *pFile = NULL;
    char *pStr = NULL;
    int rc = 0;

    do
    {
        if ((pFile = fopen(inputFileName, "r")) == NULL)
        {
            LOG_ERROR("Can't open file %s", inputFileName);
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
        free(pStr);
        return 0;
    } while (0);

    fclose(pFile);
    free(pStr);
    return -1;
}

int writeFile(void)
{
    FILE *pFile = NULL;
    char *pStr = NULL;
    int rc = 0;

    do
    {
        if ((pFile = fopen(outputFileName, "w+")) == NULL)
        {
            LOG_ERROR("Can't open file %s", outputFileName);
            break;
        }

        pStr = (char *)malloc(100);
        if (pStr == NULL)
        {
            LOG_ERROR("malloc error");
            break;
        }

        rc = fprintf(pFile, "%lu", plusResult);
        if (rc <= 0)
        {
            LOG_ERROR("write file error, %d", rc);
            break;
        }

        fclose(pFile);
        free(pStr);
        return 0;
    } while (0);

    fclose(pFile);
    free(pStr);
    return -1;
}

pthread_mutex_t m0;

void *threadHandler(void *param)
{
    while (1)
    {
        unsigned int addNumber = 0;
        pthread_mutex_lock(&m0);
        if (++pulsNumber > M)
        {
            pulsNumber--;
            pthread_mutex_unlock(&m0);
            return NULL;
        }
        addNumber = pulsNumber;
        plusResult += pulsNumber;
        pthread_mutex_unlock(&m0);
        LOG_DEBUG("add %d", addNumber);
    }
}

int main(int argc, char const *argv[])
{
    pthread_t pThread[100];
    int i = 0;
    struct timespec timeSpecStart;
    struct timespec timeSpecEnd;
    void *result;
    plusResult = 0;

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
    for (i = 0; i < N; i++)
    {
        if (pthread_join(pThread[i], &result) == -1)
        {
            LOG_ERROR("fail to recollect %d", i);
            continue;
        }
    }

    writeFile();
    LOG_DEBUG("result %lu", plusResult);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeSpecEnd);
    LOG_DEBUG("runtime %d s, %d ns", (timeSpecEnd.tv_sec - timeSpecStart.tv_sec), timeSpecEnd.tv_nsec - timeSpecStart.tv_nsec);

    return 0;
}