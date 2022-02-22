#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "helper.h"
#include "myqueue.h"

#define LOCK_PATH "/tmp/ysf_final_lock.pid"
typedef enum
{
    SELECT,
    SELECT_DISTINCT,
    UPDATE
} type;

typedef struct Query
{
    type queryType;
    int *columnIndex;
    int columnSize;

    int updateWhereIndex;
    char *selectvalue;
    char **updatevalue;

} query;

typedef struct Node
{
    char **row;
    struct Node *next;

} node;

typedef struct DataBase
{
    node *head;
    int columnSize;
    int rowSize;

} DataBase;

typedef struct QueryRespond
{
    type respondtype;
    node **rowIndex;
    int *columnIndex;
    int columnSize;
    int rowSize;
} queryRespond;

typedef struct ServerConfig
{
    char datasetPath[LINE_LEN];
    char logFilePath[LINE_LEN];

    int logFD;
    int lockFD;
    int port;
    int poolSize;
    int AR;
    int AW;
    int WR;
    int WW;
    int sockfd;
    pthread_t *threads;
    pthread_mutex_t m, qm;
    pthread_cond_t okToRead, okToWrite, qcond;
} serverConfig;
int volatile still = 1;
int busy;
void readCommandLine(int argc, char **argv);
void loadDataSet();
void freeAll();
void free_threads();

query createQuery(char *input);
void queryInfo(query query_);
void freeQuery(query query_);
void freeQueryRespond(queryRespond qr);
queryRespond getQuery(query query_);
void prepareSocket();
void sendRespond(queryRespond respond, int sockfd);
void initThreads();
void *worker(void *data);
void serverLog(char *str);
void sigHandler(int csignal);
int blockInstance(char *filename);
int become_daemon();
#endif