#ifndef HELPER_H_
#define HELPER_H_

#include <sys/time.h>

#define LINE_LEN 4096
#define SEND_LIMIT 256

typedef struct Timer
{
    struct timeval begin;
    struct timeval end;
} Timer;

void startTimer(Timer *timer);
void stopTimer(Timer *timer);
double readTimer(Timer *timer);
int findColumnCount(char *line);
void tostrlower(char *str);
void signalBlock();
void signalUnblock();
int isPendingSignal();
#endif