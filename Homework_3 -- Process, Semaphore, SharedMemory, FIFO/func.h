#ifndef FUNC_H_
#define FUNC_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <time.h>
#include <signal.h>

typedef struct Potatos
{
    int pid;
    int switchCount;
    int hotcount;
} Potato;
typedef struct SharedStruct
{
    sem_t sem2;
    int fifo_count;
    int alive_patato;
    int potatocount;
    Potato potatos[256];
} shared_struct;

void sigHandler(int csignal);
int input(int argc, char *argv[], int *hotornot, char **namememory, char **filenames, char **namesemaphore);
int createfifos(void *addr, char *filenames, int *fd_r, int fd_w[256], char fd_rn[256], char fd_wn[256][256], char *namesemaphore, sem_t *sem);
int create_shared_mem(char *namememory, void **addr);
int fill_shared_mem(void *addr);
int insert_potato(int hotcount, void *addr);
int switch_potatos(int fd_r, int fd_w[256], void *addr, int hotcount, char fd_rn[256], char fd_wn[256][256], sem_t *sem);

#endif