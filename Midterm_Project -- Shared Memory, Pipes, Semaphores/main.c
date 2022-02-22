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
#include <sys/wait.h>

typedef struct Properties
{
    int numberOfNurses;
    int numberOfVaccinators;
    int numberOfCitizens;
    int sizeOfBuffer;
    int shotsCount;
    char pathname[256];
} properties;

typedef struct SharedStruct
{
    int totalShots;
    int citizenNum;
    int shotRound;
    sem_t full, empty, m, readsem;
    int vaccineControl;
    int vaccines[2];
} shared_struct;

int fill_struct(properties *inputs, int argc, char *argv[]);
int create_process(properties *inputs, void *addr);
int create_shared_mem(void **addr);
int fill_shared_mem(void *addr, int sizeOfBuffer, int numberOfCitizen, int shotCount);
int fileread(int fd, properties *inputs, void *addr, int nurseNum);
int volatile still = 1;
void sigHandler(int csignal)
{
    still = 0;
    fprintf(stdout, "%s", "Stopped by signal `SIGINT' and all resources to return the system\n");
}
int main(int argc, char *argv[])
{
    signal(SIGINT, sigHandler);
    properties inputs;
    void *addr; // shared memory address

    if (fill_struct(&inputs, argc, argv))
        return 1;

    if (!create_shared_mem(&addr))                                                              // if memory not exist then create
        fill_shared_mem(addr, inputs.sizeOfBuffer, inputs.numberOfCitizens, inputs.shotsCount); // fill sharedMemory
    create_process(&inputs, addr);
    shared_struct *temp = addr;

    if (sem_destroy(&temp->empty) == -1)
    {
        perror("CLOSE2: ");
        exit(-1);
    }
    if (sem_destroy(&temp->full) == -1)
    {
        perror("CLOSE1: ");
        exit(-1);
    }
    if (sem_destroy(&temp->m) == -1)
    {
        perror("CLOSE3: ");
        exit(-1);
    }
    if (sem_destroy(&temp->readsem) == -1)
    {
        perror("CLOSE3: ");
        exit(-1);
    }

    if (munmap(addr, sizeof(shared_struct)) == -1)
    {
        perror("munmap: ");
        exit(-1);
    }
    while (wait(NULL) > 0)
        ;

    if (still)
        printf("The clinic is now closed.Stay healthy.\n");

    return 0;
}

int create_process(properties *inputs, void *addr)
{
    printf("Welcome to the GTU344 clinic.");
    printf("Number of citizens to vaccinate c= %d with t= %d doses.\n", inputs->numberOfCitizens, inputs->shotsCount);
    shared_struct *temp = addr;
    int fd = open(inputs->pathname, O_RDONLY);
    if (fd < 0)
    {
        perror("ERROR OPEN FILE : ");
        exit(1);
    }
    pid_t c;
    int citizens_pid[inputs->numberOfCitizens];
    int fd_citizens[inputs->numberOfCitizens][2];

    for (int i = 0; i < inputs->numberOfCitizens && still; i++)
    {
        if (-1 == pipe(fd_citizens[i]))
        {
            perror("pipe: ");
            exit(EXIT_FAILURE);
        }
        c = fork();
        citizens_pid[i] = c;

        if (c < 0)
        {
            perror("Fork: ");
        }
        else if (c == 0)
        {
            int pipes;
            close(fd_citizens[i][1]);
            for (int j = 0; j < inputs->shotsCount; j++)
            {
                read(fd_citizens[i][0], &pipes, sizeof(int));
            }
            close(fd_citizens[i][0]);
            exit(0);
        }
    }
    for (int i = 0; i < inputs->numberOfCitizens; i++)
        close(fd_citizens[i][0]);
    if (still)
    {
        for (int i = 0; i < inputs->numberOfNurses && still; i++)
        {
            c = fork();
            if (c < 0)
            {
                perror("Fork: ");
            }
            else if (c == 0)
            {
                fileread(fd, inputs, addr, i);
                exit(0);
            }
        }
    }

    for (int i = 0; i < inputs->numberOfVaccinators && still; i++)
    {
        c = fork();
        int vaccinator_count = 0;
        int temp2;
        if (c < 0)
        {
            perror("Fork: ");
        }
        else if (c == 0)
        {
            while (still)
            {
                temp2 = citizens_pid[temp->citizenNum];
                write(fd_citizens[temp->citizenNum][1], &temp2, sizeof(int));
                if (-1 == sem_wait(&temp->full))
                {
                    perror("sem wait: ");
                    exit(EXIT_FAILURE);
                }
                if (-1 == sem_wait(&temp->m))
                {
                    perror("sem wait: ");
                    exit(EXIT_FAILURE);
                }

                if (temp->totalShots != 0)
                {
                    printf("Vaccinator %d (pid= %d) is inviting citizen pid=%d to the clinic.\n",
                           i + 1, getpid(), citizens_pid[temp->citizenNum]);

                    printf("Citizen %d (pid= %d) is vaccinated for the %d. time : the clinic has %d vaccine1 and %d vaccine2.\n",
                           temp->citizenNum + 1, citizens_pid[temp->citizenNum], temp->shotRound, temp->vaccines[0] - 1, temp->vaccines[1] - 1);
                    if (temp->citizenNum == inputs->numberOfCitizens - 1)
                    {
                        temp->shotRound++;
                        temp->citizenNum = 0;
                    }
                    else
                    {
                        temp->citizenNum++;
                    }

                    vaccinator_count++;
                    temp->vaccines[0]--;
                    temp->vaccines[1]--;
                    temp->totalShots--;
                    if (temp->totalShots < inputs->numberOfCitizens)
                    {
                        //close(fd_citizens[temp->citizenNum][1]);
                        printf("The citizen is leaving.Remaining citizens to vaccinate : %d\n", temp->totalShots);
                    }

                    if (temp->totalShots == 0)
                    {
                        printf("All citizens have been vaccinated.\n");
                        if (-1 == sem_post(&temp->full))
                        {
                            perror("sem post :");
                            exit(EXIT_FAILURE);
                        }
                        if (-1 == sem_post(&temp->m))
                        {
                            perror("sem post :");
                            exit(EXIT_FAILURE);
                        }
                        if (-1 == sem_post(&temp->empty))
                        {
                            perror("sem post :");
                            exit(EXIT_FAILURE);
                        }

                        break;
                    }
                    if (-1 == sem_post(&temp->m))
                    {
                        perror("sem post :");
                        exit(EXIT_FAILURE);
                    }

                    if (-1 == sem_post(&temp->empty))
                    {
                        perror("sem post :");
                        exit(EXIT_FAILURE);
                    }
                }
                else
                {
                    if (-1 == sem_post(&temp->full))
                    {
                        perror("sem post :");
                        exit(EXIT_FAILURE);
                    }

                    if (-1 == sem_post(&temp->m))
                    {
                        perror("sem post :");
                        exit(EXIT_FAILURE);
                    }

                    break;
                }
            }

            printf("Vaccinator %d (pid=%d) vaccinated %d doses.\n", i + 1, getpid(), vaccinator_count);
            exit(0);
        }
    }
    for (int i = 0; i < inputs->numberOfCitizens; i++)
        close(fd_citizens[i][1]);
    return 0;
}

int fileread(int fd, properties *inputs, void *addr, int nurseNum)
{
    shared_struct *temp = addr;
    int count = 1;
    char c = ' ';
    int control = 0;
    while (count != 0 && still)
    {
        if (-1 == sem_wait(&temp->readsem))
        {
            perror("sem wait: ");
            exit(EXIT_FAILURE);
        }
        count = read(fd, &c, 1);
        if (-1 == sem_post(&temp->readsem))
        {
            perror("sem post: ");
            exit(EXIT_FAILURE);
        }

        if (count > 0 && (c == '1' || c == '2'))
        {
            if (-1 == sem_wait(&temp->empty))
            {
                perror("sem wait: ");
                exit(EXIT_FAILURE);
            }

            if (-1 == sem_wait(&temp->m))
            {
                perror("sem wait: ");
                exit(EXIT_FAILURE);
            }

            if (c == '1')
            {
                temp->vaccines[0]++;
                if (temp->vaccines[0] <= temp->vaccines[1])
                    control = 1;
            }
            else
            {
                temp->vaccines[1]++;
                if (temp->vaccines[1] <= temp->vaccines[0])
                    control = 1;
            }
            printf("Nurse %d (pid=%d) has brought vaccine %c: the clinic has %d vaccine1 and %d vaccine2.\n", nurseNum + 1, getpid(), c, temp->vaccines[0], temp->vaccines[1]);

            if (-1 == sem_post(&temp->m))
            {
                perror("sem post: ");
                exit(EXIT_FAILURE);
            }
            if (control == 1)
            {
                control = 0;

                if (-1 == sem_post(&temp->full))
                {
                    perror("sem post: ");
                    exit(EXIT_FAILURE);
                }
            }
        }
        if (count == 0 && still)
        {
            if (temp->vaccineControl == inputs->numberOfNurses - 1 && still)
            {
                temp->vaccineControl++;
                printf("Nurses have carried all vaccines to the buffer, terminating.\n");
            }
            else
                temp->vaccineControl++;
        }
    }

    return 0;
}

int create_shared_mem(void **addr)
{

    *addr = mmap(NULL, sizeof(shared_struct), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (*addr == MAP_FAILED)
    {
        perror("mmap: ");
        exit(1);
    }
    return 0;
}

int fill_shared_mem(void *addr, int sizeOfBuffer, int numberOfCitizen, int shotCount)
{

    shared_struct mystruct;

    if (sem_init(&mystruct.m, 1, 1) == -1)
    {
        perror("sem init2 : ");
    }
    if (sem_init(&mystruct.empty, 1, sizeOfBuffer) == -1)
    {
        perror("sem init3 : ");
    }
    if (sem_init(&mystruct.readsem, 1, 1) == -1)
    {
        perror("sem init4 : ");
    }
    if (sem_init(&mystruct.full, 1, 0) == -1)
    {
        perror("sem init : ");
    }
    mystruct.vaccines[0] = 0;
    mystruct.vaccines[1] = 0;
    mystruct.vaccineControl = 0;
    mystruct.totalShots = numberOfCitizen * shotCount;
    mystruct.shotRound = 1;
    mystruct.citizenNum = 0;
    memcpy(addr, &mystruct, sizeof(shared_struct));
    return 0;
}

int fill_struct(properties *inputs, int argc, char *argv[])
{
    int opt;
    int control = 0;
    int n = 0;
    int v = 0;
    int c = 0;
    int b = 0;
    int t = 0;
    int i = 0;

    while ((opt = getopt(argc, argv, "n:v:c:b:t:i:")) != -1)
    {
        switch (opt)
        {
        case 'n':
            if (atoi(optarg) >= 2)
            {
                n++;
                inputs->numberOfNurses = atoi(optarg);
            }
            else
            {
                printf("Error: Number of nurses count must be greater than 1!\n");
                return 1;
            }
            break;
        case 'v':
            if (atoi(optarg) >= 2)
            {
                v++;
                inputs->numberOfVaccinators = atoi(optarg);
            }
            else
            {
                printf("Error: Number of vaccinators count must be greater than 1!\n");
                return 1;
            }
            break;
        case 'c':
            if (atoi(optarg) >= 3)
            {
                c++;
                inputs->numberOfCitizens = atoi(optarg);
            }
            else
            {
                printf("Error:Number of citizens count must be greater than 2!\n");
                return 1;
            }
            break;

        case 'b':
            b++;
            inputs->sizeOfBuffer = atoi(optarg);
            break;
        case 't':
            if (atoi(optarg) >= 1)
            {
                t++;
                inputs->shotsCount = atoi(optarg);
            }
            else
            {
                printf("Error: Shots count must be greater than 0!\n");
                return 1;
            }
            break;
        case 'i':
            i++;
            strcpy(inputs->pathname, optarg);
            break;
        case '?':
            fprintf(stderr, "%s %c %s", "unknown option:", optopt, "\n");
            return 1;
        }
    }
    control = n + v + c + b + t + i;
    if (control < 6)
    {
        fprintf(stderr, "%s", "Error: At least 1 parameter was missing.\n");
        return 1;
    }
    if (control > 6)
    {
        fprintf(stderr, "%s", "Error: One or more parameters have been entered more than once.\n");
        return 1;
    }
    if (inputs->sizeOfBuffer < (inputs->shotsCount + inputs->numberOfCitizens + 1))
    {
        fprintf(stderr, "%s", "Error: Criteria b must be greater than t*c+1\n");
        return 1;
    }

    return 0;
}