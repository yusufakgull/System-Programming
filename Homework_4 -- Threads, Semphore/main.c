#include "func.h"
#include "myqueue.h"

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        printf("Invalid command line arguments!\n");
        return 1;
    }
    still = 1;
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    if (sem_init(&sem_queue, 0, 1) == -1)
    {
        perror("Sem init : ");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_empty, 0, SIZE) == -1)
    {
        perror("Sem init : ");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_full, 0, 0) == -1)
    {
        perror("Sem init : ");
        exit(EXIT_FAILURE);
    }
    if (sem_init(&sem_money, 0, 1) == -1)
    {
        perror("Sem init : ");
        exit(EXIT_FAILURE);
    }

    student students[MAX_STUDENT_SIZE];
    int speeds[MAX_STUDENT_SIZE];
    int qualitys[MAX_STUDENT_SIZE];
    int prices[MAX_STUDENT_SIZE];
    int student_count = 0;
    int minPrice;
    int fd_hw = open(argv[1], O_RDONLY);
    if (fd_hw < 0)
    {
        perror("ERROR OPEN FILE : ");
        exit(EXIT_FAILURE);
    }

    money = atoi(argv[3]);

    readFromFileStudents(students, &student_count, argv[2]);
    sortStudentsByProperties(students, speeds, qualitys, prices, student_count);
    minPrice = students[prices[0]].price;
    printf("%d students-for-hire threads have been created.\nName Q S C\n", student_count);
    for (int i = 0; i < student_count; i++)
    {
        printf("%s %d %d %d\n", students[i].name, students[i].quality, students[i].speed, students[i].price);
    }
    if (sem_init(&sem_all_students, 0, student_count) == -1)
    {
        perror("Sem init : ");
        exit(EXIT_FAILURE);
    }

    pthread_t studentsThreads[student_count];
    for (int i = 0; i < student_count && still; i++)
    {
        pthread_create(&studentsThreads[i], NULL, &studentsDoingHw, &students[i]);
    }

    pthread_t h;
    pthread_create(&h, NULL, &readFromFileHw, &fd_hw);
    char c;
    int hwControl = 1;
    int checkHwDone = 0;
    int temp_money = money;
    while (hwControl == 1 && still)
    {

        if (sem_wait(&sem_full) == -1 && errno != EINTR)
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        if (sem_wait(&sem_queue) == -1 && errno != EINTR)
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        c = deleteFromQueue();

        if (sem_wait(&sem_money) == -1 && errno != EINTR)
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        if (money < minPrice && c != 'F')
        {
            money = 0;
            hwControl = -1;
        }

        if (sem_post(&sem_money) == -1)
        {
            perror("sem post: ");
            exit(EXIT_FAILURE);
        }

        if (sem_post(&sem_queue) == -1)
        {
            perror("sem post: ");
            exit(EXIT_FAILURE);
        }
        if (sem_post(&sem_empty) == -1)
        {
            perror("sem post: ");
            exit(EXIT_FAILURE);
        }
        if (sem_wait(&sem_all_students) == -1 && errno != EINTR)
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        if (money >= minPrice && still)
        {
            switch (c)
            {
            case 'C':
                for (int i = 0; i < student_count && still; i++)
                {
                    if (students[prices[i]].available == 1 && money >= students[prices[i]].price)
                    {
                        checkHwDone = 1;
                        students[prices[i]].available = 0;
                        if (sem_wait(&sem_money) == -1 && errno != EINTR)
                        {
                            perror("sem wait: ");
                            exit(EXIT_FAILURE);
                        }
                        money -= students[prices[i]].price;
                        students[prices[i]].count += 1;
                        printf("%s is solving homework C for %d, H has %dTL left.\n", students[prices[i]].name, students[prices[i]].price, money);
                        if (sem_post(&sem_money) == -1)
                        {
                            perror("sem post: ");
                            exit(EXIT_FAILURE);
                        }
                        if (sem_post(&students[prices[i]].sem_student) == -1)
                        {
                            perror("sem post: ");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    }
                }

                break;
            case 'S':
                for (int i = 0; i < student_count && still; i++)
                {
                    if (students[speeds[i]].available == 1 && money >= students[speeds[i]].price)
                    {
                        checkHwDone = 1;
                        students[speeds[i]].available = 0;
                        if (sem_wait(&sem_money) == -1 && errno != EINTR)
                        {
                            perror("sem wait: ");
                            exit(EXIT_FAILURE);
                        }
                        money -= students[speeds[i]].price;
                        students[speeds[i]].count += 1;
                        if (sem_post(&sem_money) == -1)
                        {
                            perror("sem post: ");
                            exit(EXIT_FAILURE);
                        }
                        printf("%s is solving homework S for %d, H has %dTL left.\n", students[speeds[i]].name, students[speeds[i]].price, money);
                        fflush(stdout);
                        if (sem_post(&students[speeds[i]].sem_student) == -1)
                        {
                            perror("sem post: ");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    }
                }
                break;
            case 'Q':
                for (int i = 0; i < student_count && still; i++)
                {
                    if (students[qualitys[i]].available == 1 && money >= students[qualitys[i]].price)
                    {
                        checkHwDone = 1;
                        students[qualitys[i]].available = 0;
                        if (sem_wait(&sem_money) == -1 && errno != EINTR)
                        {
                            perror("sem wait: ");
                            exit(EXIT_FAILURE);
                        }
                        money -= students[qualitys[i]].price;
                        students[qualitys[i]].count += 1;
                        if (sem_post(&sem_money) == -1)
                        {
                            perror("sem post: ");
                            exit(EXIT_FAILURE);
                        }
                        printf("%s is solving homework Q for %d, H has %dTL left.\n", students[qualitys[i]].name, students[qualitys[i]].price, money);
                        fflush(stdout);
                        if (sem_post(&students[qualitys[i]].sem_student) == -1)
                        {
                            perror("sem post: ");
                            exit(EXIT_FAILURE);
                        }
                        break;
                    }
                }
                break;

            case 'F':
                if (sem_wait(&sem_money) == -1)
                {
                    perror("sem wait: ");
                    exit(EXIT_FAILURE);
                }
                money = 0;
                if (sem_post(&sem_money) == -1)
                {
                    perror("sem post: ");
                    exit(EXIT_FAILURE);
                }
                hwControl = 0;
                break;
            default:
                break;
            }
        }

        if (checkHwDone == 0 && hwControl != 0)
        {
            if (money < minPrice)
                hwControl = -1;
            else
                hwControl = -2;
        }
        else
        {
            checkHwDone = 0;
        }
    }
    if (sem_wait(&sem_money) == -1)
    {
        perror("sem wait: ");
        exit(EXIT_FAILURE);
    }
    money = 0;
    if (sem_post(&sem_money) == -1)
    {
        perror("sem post: ");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < student_count; i++)
    {
        sem_post(&students[i].sem_student);
    }
    if (still)
    {
        if (hwControl == 0)
        {
            printf("No more homeworks left or coming in, closing.\n");
        }
        else if (hwControl < 0)
        {
            if (sem_wait(&sem_money) == -1 && errno != EINTR)
            {
                perror("sem wait: ");
                exit(EXIT_FAILURE);
            }
            money = 0;
            if (sem_post(&sem_money) == -1)
            {
                perror("sem post: ");
                exit(EXIT_FAILURE);
            }
            if (hwControl == -1)
                printf("Money is over, closing.\n");
            else
                printf("There is not enough money for available students, closing.\n");
            if (sem_post(&sem_empty) == -1)
            {
                perror("sem post: ");
                exit(EXIT_FAILURE);
            }
            if (sem_post(&sem_queue) == -1)
            {
                perror("sem post: ");
                exit(EXIT_FAILURE);
            }
        }
    }
    else
    {
        fprintf(stdout, "%s", "Termination signal received, closing.\n");
    }
    for (int i = 0; i < student_count; i++)
    {
        pthread_join(studentsThreads[i], NULL);
    }
    printf("Homeworks solved and money made by the students:\n");
    int totalhw = 0;
    int totalhwcost = 0;
    for (int i = 0; i < student_count; i++)
    {
        printf("%s %d %d\n", students[i].name, students[i].count, students[i].count * students[i].price);
        totalhw += students[i].count;
        totalhwcost += students[i].count * students[i].price;
    }
    printf("Total cost for %d homeworks %dTL\n", totalhw, totalhwcost);
    printf("Money left at G's account: %dTL\n", temp_money - totalhwcost);

    if (sem_destroy(&sem_queue) == -1)
    {
        perror("Sem destroy : ");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&sem_empty) == -1)
    {
        perror("Sem destroy : ");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&sem_full) == -1)
    {
        perror("Sem destroy : ");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&sem_all_students) == -1)
    {
        perror("Sem destroy : ");
        exit(EXIT_FAILURE);
    }
    if (sem_destroy(&sem_money) == -1)
    {
        perror("Sem destroy : ");
        exit(EXIT_FAILURE);
    }
    return 0;
}
