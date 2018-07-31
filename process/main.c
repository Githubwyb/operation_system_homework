#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

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

int main(int argc, char const *argv[])
{
    int pid = -1;
    int i = 0;
    struct timespec timeSpecStart;
    struct timespec timeSpecEnd;

    plusResult = 0;

    LOG_DEBUG("Hello, gcc\n");

    if (parseFile() != 0)
    {
        LOG_ERROR("parse file error");
        return -1;
    }
    LOG_DEBUG("parse file success N = %d, M = %d", N, M);

    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeSpecStart);

    for (i = 0; i < N - 1; i++)
    {
        if (pid != 0)
        {
            while ((pid = fork()) == -1)
                ;
        }
    }

    // LOG_DEBUG("pid %d", pid);

    while (1)
    {
        unsigned int addNumber = __sync_add_and_fetch(&pulsNumber, 1);
        if (addNumber > M)
        {
            __sync_sub_and_fetch(&pulsNumber, 1);
            break;
        }

        __sync_add_and_fetch(&plusResult, addNumber);
        LOG_DEBUG("add %d", addNumber);
    }

    if (pid != 0)
    {
        waitpid(0, NULL, 0);
        writeFile();
        clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &timeSpecEnd);

        LOG_DEBUG("result %lu", plusResult);
        LOG_DEBUG("runtime %d s, %d ns", (timeSpecEnd.tv_sec - timeSpecStart.tv_sec), timeSpecEnd.tv_nsec - timeSpecStart.tv_nsec);
    }

    return 0;
}