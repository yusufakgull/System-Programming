#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include "helper.h"

int findColumnCount(char *line)
{
    char abc[strlen(line) + 1];
    strcpy(abc, line);
    char *token = strtok(abc, ",");
    int count = 0;
    while (token != NULL)
    {
        count++;
        // printf("Token: %s\n", token);
        token = strtok(NULL, ",");
    }
    return count;
}
void startTimer(Timer *timer)
{
    if (-1 == gettimeofday(&timer->begin, NULL))
    {
        perror("startTimer, gettimeofday");
        exit(EXIT_FAILURE);
    }
}

void stopTimer(Timer *timer)
{
    if (-1 == gettimeofday(&timer->end, NULL))
    {
        perror("stopTimer, gettimeofday");
        exit(EXIT_FAILURE);
    }
}

double readTimer(struct Timer *timer)
{
    time_t sDiff = timer->end.tv_sec - timer->begin.tv_sec;
    suseconds_t uDiff = timer->end.tv_usec - timer->begin.tv_usec;
    double diff = (double)sDiff + ((double)uDiff / 1000000.0);
    return diff;
}

void tostrlower(char *str)
{
    for (; *str; ++str)
        *str = tolower(*str);
}
