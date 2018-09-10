#include "log.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <errno.h>

const char *const inputFileName = "input.txt";
const char *const outputFileName = "output.txt";
static unsigned int N = 0;
static unsigned int M = 0;

typedef struct
{
    unsigned int pulsNumber;
    unsigned long int plusResult;
} Plus;

// struct sembuf
// {
//     short semnum; /*信号量集合中的信号量编号，0代表第1个信号量*/
//     short val;    /*若val>0进行V操作信号量值加val，表示进程释放控制的资源 */
//                   /*若val<0进行P操作信号量值减val，若(semval-val)<0（semval为该信号量值），则调用进程阻塞，直到资源可用；若设置IPC_NOWAIT不会睡眠，进程直接返回EAGAIN错误*/
//                   /*若val==0时阻塞等待信号量为0，调用进程进入睡眠状态，直到信号值为0；若设置IPC_NOWAIT，进程不会睡眠，直接返回EAGAIN错误*/
//     short flag;   /*0 设置信号量的默认操作*/
//                   /*IPC_NOWAIT设置信号量操作不等待*/
//                   /*SEM_UNDO 选项会让内核记录一个与调用进程相关的UNDO记录，如果该进程崩溃，则根据这个进程的UNDO记录自动恢复相应信号量的计数值*/
// };

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
    Plus *pPlus = NULL;

    LOG_DEBUG("Hello, gcc");

    if (parseFile() != 0)
    {
        LOG_ERROR("parse file error");
        return -1;
    }
    LOG_DEBUG("parse file success N = %d, M = %d", N, M);

    gettimeofday(&timeSpecStart, NULL);

    int shmid = shmget((key_t)1234, sizeof(Plus), 0666 | IPC_CREAT);
    if (shmid == -1)
    {
        LOG_ERROR("shmget failed");
        return -1;
    }

    key_t key = ftok(".", 0x01);
    if (key < 0)
    {
        LOG_ERROR("ftok failed, key %d", key);
        return -1;
    }

    int semId = semget(key, 1, IPC_CREAT | 0600);
    if (semId == -1)
    {
        LOG_ERROR("semget failed");
        return -1;
    }

    int code = semctl(semId, 0, SETVAL, 1);
    if (code == -1)
    {
        LOG_ERROR("semctl failed");
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

    //将共享内存连接到当前进程的地址空间
    pPlus = (Plus *)shmat(shmid, (void *)0, 0);
    if (pPlus == (Plus *)-1)
    {
        LOG_ERROR("shmat fail");
        return -1;
    }

    while (1)
    {
        struct sembuf signal;
        signal.sem_op = -1;
        signal.sem_flg = SEM_UNDO;
        signal.sem_num = 0;
        code = semop(semId, &signal, 1);
        if (code == -1)
        {
            LOG_ERROR("semop failed, %d, %s", errno, strerror(errno));
            return -1;
        }

        // unsigned int addNumber = __sync_add_and_fetch(&(pPlus->pulsNumber), 1);
        // if (addNumber > M)
        // {
        //     __sync_sub_and_fetch(&(pPlus->pulsNumber), 1);
        //     break;
        // }

        // __sync_add_and_fetch(&(pPlus->plusResult), addNumber);

        pPlus->pulsNumber++;
        if (pPlus->pulsNumber > M)
        {
            pPlus->pulsNumber--;
            signal.sem_op = 1;
            code = semop(semId, &signal, 1);
            if (code == -1)
            {
                LOG_ERROR("semop failed");
                return -1;
            }
            break;
        }
        pPlus->plusResult += pPlus->pulsNumber;

        signal.sem_op = 1;
        code = semop(semId, &signal, 1);
        if (code == -1)
        {
            LOG_ERROR("semop failed");
            return -1;
        }
    }

    if (pid != 0)
    {
        for (i = 0; i < N - 1; i++)
        {
            int status = 0;
            pid_t pr = wait(&status);

            if (WIFEXITED(status))
            { /* 如果WIFEXITED返回非零值 */
                if (WEXITSTATUS(status) != 0)
                {
                    LOG_DEBUG("the return code is %d.", WEXITSTATUS(status));
                }
            }
            else
            {
                LOG_DEBUG("the child process %d exit abnormally.", pr);
            }
        }

        gettimeofday(&timeSpecEnd, NULL);

        LOG_DEBUG("result %lu", pPlus->plusResult);
        LOG_DEBUG("runtime %lu us", (unsigned long int)((timeSpecEnd.tv_sec - timeSpecStart.tv_sec) * 1000000 + (timeSpecEnd.tv_usec - timeSpecStart.tv_usec)));

        writeFile(pPlus->plusResult);
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

        LOG_DEBUG("OK");
        int code = semctl(semId, 0, IPC_RMID);
        if (code == -1)
        {
            LOG_ERROR("semctl failed");
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