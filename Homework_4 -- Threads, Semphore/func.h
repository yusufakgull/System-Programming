#ifndef FUNC_H_
#define FUNC_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

typedef struct Students
{
    char name[256];
    int quality;
    int speed;
    int price;
    int available;
    int count;
    sem_t sem_student;
} student;

#define SIZE 4 // queue size
#define MAX_STUDENT_SIZE 512
sem_t sem_queue, sem_full, sem_empty, sem_all_students, sem_money;
int money;
int volatile still;

void *readFromFileHw(void *fd_hw);
void *studentsDoingHw(void *student_);
int readFromFileStudents(student *students, int *student_count, char *filename);
int sortStudentsByProperties(student *students, int *speeds, int *qualitys, int *prices, int student_count);
void sigHandler(int csignal);

#endif