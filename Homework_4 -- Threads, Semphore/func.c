#include "func.h"
#include "myqueue.h"

void sigHandler(int csignal)
{
    still = 0;
}
int readFromFileStudents(student *students, int *student_count, char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        perror("file open: ");
        exit(EXIT_FAILURE);
    }
    char line[256];
    while (fgets(line, sizeof(line), fp) != NULL && still)
    {
        strcpy(students[*student_count].name, strtok(line, " "));
        students[*student_count].quality = atoi(strtok(NULL, " "));
        students[*student_count].speed = atoi(strtok(NULL, " "));
        students[*student_count].price = atoi(strtok(NULL, " "));
        students[*student_count].count = 0;
        students[*student_count].available = 1;
        if (sem_init(&students[*student_count].sem_student, 0, 0) == -1)
        {
            perror("Sem init : ");
            exit(EXIT_FAILURE);
        }
        *student_count += 1;
    }
    fclose(fp);
    return 0;
}

void *studentsDoingHw(void *student_)
{
    student *st = (student *)student_;
    int temp_money = 1;
    st->available = 1;
    while (temp_money != 0 && still)
    {
        printf("%s is waiting for a homework\n", st->name);
        if (sem_wait(&st->sem_student) == -1)
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        if (sem_wait(&sem_money) == -1)
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        temp_money = money;
        if (sem_post(&sem_money) == -1)
        {
            perror("sem post: ");
            exit(EXIT_FAILURE);
        }
        if (temp_money != 0 && still)
        {
            sleep(6 - st->speed);
            st->available = 1;
        }
        if (sem_wait(&sem_money) == -1)
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        temp_money = money;
        if (sem_post(&sem_money) == -1)
        {
            perror("sem post: ");
            exit(EXIT_FAILURE);
        }
        if (sem_post(&sem_all_students) == -1)
        {
            perror("sem post: ");
            exit(EXIT_FAILURE);
        }
    }
    return NULL;
}
int sortStudentsByProperties(student *students, int *speeds, int *qualitys, int *prices, int student_count)
{
    int count = 0;
    for (int i = 0; i < student_count; i++)
    {
        speeds[i] = -1;
        qualitys[i] = -1;
        prices[i] = -1;
    }

    for (int i = 0; i < student_count && still; i++)
    {
        for (int j = i + 1; j < student_count && still; j++)
        {
            if (students[i].speed < students[j].speed)
                count++;
        }
        while (speeds[count] != -1)
            count++;
        speeds[count] = i;
        count = 0;
    }

    for (int i = 0; i < student_count && still; i++)
    {
        for (int j = i + 1; j < student_count && still; j++)
        {
            if (students[i].quality < students[j].quality)
                count++;
        }
        while (qualitys[count] != -1)
            count++;
        qualitys[count] = i;
        count = 0;
    }
    for (int i = 0; i < student_count && still; i++)
    {
        for (int j = i + 1; j < student_count && still; j++)
        {
            if (students[i].price > students[j].price)
                count++;
        }
        while (prices[count] != -1)
            count++;
        prices[count] = i;
        count = 0;
    }
    return 0;
}
void *readFromFileHw(void *fd_hw)
{

    if (pthread_detach(pthread_self()) != 0)
    {
        printf("error pthread detach!");
        exit(EXIT_FAILURE);
    }
    int fd = *(int *)fd_hw;
    char c = ' ';
    int count = 1;
    int tempmoney = 1;
    while (count != 0 && tempmoney > 0 && still)
    {
        count = read(fd, &c, 1);
        if (count > 0)
        {
            if (c != 'C' && c != 'S' && c != 'Q')
            {
                printf("Invalid input in file.(Input characters must be C, S, Q.\n");
                exit(EXIT_FAILURE);
            }
            else
            {
                if (sem_wait(&sem_empty) == -1)
                {
                    perror("sem wait: ");
                    exit(EXIT_FAILURE);
                }
                if (sem_wait(&sem_queue) == -1)
                {
                    perror("sem wait: ");
                    exit(EXIT_FAILURE);
                }
                if (sem_wait(&sem_money) == -1)
                {
                    perror("sem wait: ");
                    exit(EXIT_FAILURE);
                }
                tempmoney = money;
                if (sem_post(&sem_money) == -1)
                {
                    perror("sem post: ");
                    exit(EXIT_FAILURE);
                }
                if (tempmoney > 0 && still)
                {
                    insertToQueue(c);
                    printf("H has a new homework %c; remaining money is %dTL\n", c, money);
                    fflush(stdout);
                }
                else if (!still)
                {
                    //printf("H intterrupted by ctrl c signal, terminating.\n");
                }
                else
                {
                    printf("H has no enough money for homeworks, terminating.\n");
                }
                if (sem_post(&sem_queue) == -1)
                {
                    perror("sem post: ");
                    exit(EXIT_FAILURE);
                }
                if (sem_post(&sem_full) == -1)
                {
                    perror("sem post: ");
                    exit(EXIT_FAILURE);
                }
            }
        }
        else if (count == 0 && still)
        {
            if (sem_wait(&sem_empty) == -1)
            {
                perror("sem wait: ");
                exit(EXIT_FAILURE);
            }
            if (sem_wait(&sem_queue) == -1)
            {
                perror("sem wait: ");
                exit(EXIT_FAILURE);
            }
            insertToQueue('F');
            if (sem_post(&sem_queue) == -1)
            {
                perror("sem post: ");
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_full) == -1)
            {
                perror("sem post: ");
                exit(EXIT_FAILURE);
            }
            printf("H has no other homeworks, terminating.\n");
        }
    }
    close(fd);
    return NULL;
}