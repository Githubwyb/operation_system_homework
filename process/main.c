#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/time.h>

const char *const inputFileName = "input.txt";
const char *const outputFileName = "output.txt";
static unsigned int N = 0;
static unsigned int M = 0;

typedef struct
{
    unsigned int pulsNumber;
    unsigned long int plusResult;
} Plus;

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

int writeFile(unsigned long int plusResult)
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
    struct timeval timeSpecStart;
    struct timeval timeSpecEnd;
    int shmid = 0;
    Plus *pPlus = NULL;

    LOG_DEBUG("Hello, gcc");

    if (parseFile() != 0)
    {
        LOG_ERROR("parse file error");
        return -1;
    }
    LOG_DEBUG("parse file success N = %d, M = %d", N, M);

    gettimeofday(&timeSpecStart, NULL);

    shmid = shmget((key_t)1234, sizeof(Plus), 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        LOG_ERROR("shmget failed");
        return -1;
    }

    for (i = 0; i < N - 1; i++)
    {
        if (pid != 0)
        {
            while ((pid = fork()) == -1)
                ;
        }
    }

    // LOG_DEBUG("pid %d", pid);

    //将共享内存连接到当前进程的地址空间
    pPlus = (Plus *)shmat(shmid, (void *)0, 0);
    if (pPlus == (Plus *)-1)
    {
        LOG_ERROR("shmat fail");
        return -1;
    }

    while (1)
    {
        unsigned int addNumber = __sync_add_and_fetch(&(pPlus->pulsNumber), 1);
        if (addNumber > M)
        {
            __sync_sub_and_fetch(&(pPlus->pulsNumber), 1);
            break;
        }

        __sync_add_and_fetch(&(pPlus->plusResult), addNumber);
        // LOG_DEBUG("add %d", addNumber);
    }

    if (pid != 0)
    {
        waitpid(0, NULL, 0);
        writeFile(pPlus->plusResult);
        gettimeofday(&timeSpecEnd, NULL);

        LOG_DEBUG("result %lu", pPlus->plusResult);
        LOG_DEBUG("runtime %lu us", (unsigned long int)((timeSpecEnd.tv_sec - timeSpecStart.tv_sec) * 1000000 + (timeSpecEnd.tv_usec - timeSpecStart.tv_usec)));
        //把共享内存从当前进程中分离
        if (shmdt(pPlus) == -1)
        {
            LOG_ERROR("shmdt failed");
            return -1;
        }

        //删除共享内存
        if (shmctl(shmid, IPC_RMID, 0) == -1)
        {
            LOG_ERROR("shmctl(IPC_RMID) failed");
            return -1;
        }
    }
    else
    {
        //把共享内存从当前进程中分离
        if (shmdt(pPlus) == -1)
        {
            LOG_ERROR("shmdt failed");
            return -1;
        }
    }

    return 0;
}