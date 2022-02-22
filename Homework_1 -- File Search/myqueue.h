#ifndef MYQUEUE_H_
#define MYQUEUE_H_

#include "func.h"

void insertRear(char **path, char *item, int *pfront, int *prear, int *pcapacity, int *pcount);
char *removeFront(char **path, int *pfront, int *prear,int *pcount);
char *removeRear(char **path, int *front, int *rear,int *pcount);

#endif