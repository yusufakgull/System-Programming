#include "func.h"

int volatile still = 1;

void sigHandler(int csignal)
{
    still = 0;
    fprintf(stdout, "%s", "Stopped by signal `SIGINT' and all resources to return the system\n");
}

int switch_potatos(int fd_r, int fd_w[256], void *addr, int hotcount, char fd_rn[256], char fd_wn[256][256], sem_t *sem)
{
    shared_struct *temp = addr;
    int index, index2;
    int random;
    int message;

    if (hotcount > 0)
    {

        for (int i = 0; i < temp->potatocount; i++)
        {
            if (getpid() == temp->potatos[i].pid)
            {
                index = i;
                break;
            }
        }
        random = rand() % (temp->fifo_count - 1);
        sem_wait(&temp->sem2);
        temp->potatos[index].switchCount++;
        temp->potatos[index].hotcount--;
        printf("pid=%d sending potato number %d to %s ; this is switch number %d\n", getpid(), temp->potatos[index].pid, fd_wn[random], temp->potatos[index].switchCount);
        if (write(fd_w[random], &temp->potatos[index].pid, sizeof(int)) == -1)
        {
            perror("switch patatos: ");
            return 2;
        }
        sem_post(&temp->sem2);
    }
    while (temp->alive_patato != 0)
    {
        if (temp->alive_patato == 0)
            break;

        if (read(fd_r, &message, sizeof(int)) == -1)
        {
            perror("read : ");
            return 2;
        }
        else
        {
            if (message == -2 || message == -3)
            {
                if (close(fd_r) == -1)
                {
                    perror("close : ");
                }
                for (int k = 0; k < temp->fifo_count; k++)
                {
                    if (close(fd_w[k]) == -1)
                    {
                        perror("close : ");
                    }
                }

                break;
            }
            for (int i = 0; i < temp->potatocount; i++)
            {
                if (message == temp->potatos[i].pid)
                {
                    index2 = i;
                    break;
                }
            }
            printf("pid=%d receiving potato number %d from %s\n", getpid(), temp->potatos[index2].pid, fd_rn);
        }
        if (temp->potatos[index2].hotcount < 1 || still == 0)
        {
            if (still == 0)
            {
                message = -3;
                for (int i = 0; i < temp->fifo_count - 1; i++)
                    write(fd_w[i], &message, sizeof(int));
                break;
            }
            printf("pid=%d; potato number %d has cooled down.\n", getpid(), temp->potatos[index2].pid);
            sem_wait(sem);
            temp->alive_patato--;
            if (temp->alive_patato == 0)
            {

                message = -2;
                for (int i = 0; i < temp->fifo_count - 1; i++)
                    write(fd_w[i], &message, sizeof(int));
                sem_post(sem);
                break; // butun patatesler bittiyse
            }
            sem_post(sem);
        }
        else if (still == 1)
        {
            random = rand() % (temp->fifo_count - 1);
            sem_wait(sem);
            temp->potatos[index2].switchCount++;
            temp->potatos[index2].hotcount--;
            printf("pid=%d sending potato number %d to %s ; this is switch number %d\n", getpid(), temp->potatos[index2].pid, fd_wn[random], temp->potatos[index2].switchCount);
            if (write(fd_w[random], &temp->potatos[index2].pid, sizeof(int)) == -1)
            {
                perror("switch patatos: ");
                return 2;
            }
            sem_post(sem);
        }
        else
        {
            message = -3;
        }
    }
    return 0;
}

int insert_potato(int hotcount, void *addr)
{

    shared_struct *temp = addr;
    Potato patoto;
    patoto.hotcount = hotcount;
    patoto.pid = getpid();
    patoto.switchCount = 0;
    sem_wait(&temp->sem2);
    temp->potatos[temp->potatocount] = patoto;
    temp->potatocount++;
    temp->alive_patato++;
    sem_post(&temp->sem2);
    return 0;
}
int fill_shared_mem(void *addr)
{
    shared_struct mystruct;
    mystruct.alive_patato = 0;
    mystruct.potatocount = 0;
    mystruct.fifo_count = 0;
    if (sem_init(&mystruct.sem2, 1, 1) == -1)
    {
        perror("sem init : ");
    }
    memcpy(addr, &mystruct, sizeof(shared_struct));
    return 0;
}
int create_shared_mem(char *namememory, void **addr)
{

    int fd;
    int size = 512;
    int control = 0; //not exist
    fd = shm_open(namememory, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    if (fd == -1 && errno == EEXIST)
    {
        fd = shm_open(namememory, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        control = 1;
    }
    if (fd == -1)
    {
        perror("Shared memory: ");
        exit(-1);
    }

    if (control == 0)
    {
        if (ftruncate(fd, size) == -1)
        {
            perror("size : ");
            exit(-1);
        }
    }
    *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (*addr == MAP_FAILED)
        perror("mmap: ");
    if (close(fd) == -1)
        perror("close : ");

    return control;
}

int createfifos(void *addr, char *filenames, int *fd_r, int fd_w[256], char fd_rn[256], char fd_wn[256][256], char *namesemaphore, sem_t *sem)
{
    shared_struct *temp = addr;
    int i = 0;
    int j = 0;
    FILE *fp = fopen(filenames, "r");
    char line[256];
    int control = 0;
    sem_wait(sem);
    int count = temp->fifo_count;
    sem_post(sem);
    if (fp == NULL)
    {
        perror("Error opening file : ");
        return -1;
    }
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        if (line[strlen(line) - 1] == '\n')
            line[strlen(line) - 1] = '\0';
        control++;
        if (mkfifo(line, S_IRUSR | S_IWUSR) == -1)
        {
            if (errno != EEXIST)
                perror("mkfifo : ");
        }

        if (i != count)
        {
            strcpy(fd_wn[j], line);
            fd_w[j] = open(line, O_WRONLY);
            if (fd_w[j] == -1)
            {
                return 1;
            }
            j++;
        }
        else
        {
            strcpy(fd_rn, line);
            sem_wait(sem);
            temp->fifo_count++;
            sem_post(sem);
            *fd_r = open(line, O_RDONLY);
            if (*fd_r == -1)
            {
                return 1;
            }
            control = 0;
        }
        i++;
    }
    if (control == 0)
    {
        for (i = 0; i < j; i++)
            sem_post(&temp->sem2);
    }
    else
    {
        sem_wait(&temp->sem2);
    }
    fclose(fp);

    return 0;
}

int input(int argc, char *argv[], int *hotornot, char **namememory, char **filenames, char **namesemaphore)
{

    int opt;
    int b = -1;
    int s = -1;
    int f = -1;
    int m = -1;
    while ((opt = getopt(argc, argv, "b:s:f:m:")) != -1)
    {
        switch (opt)
        {
        case 'b':
            *hotornot = atoi(optarg);
            b++;
            break;
        case 's':
            *namememory = (char *)malloc(strlen(optarg) + 1 * sizeof(char));
            if (namememory == NULL)
            {
                perror("Error: ");
                exit(-1);
            }
            strcpy(*namememory, optarg);
            s++;
            break;
        case 'f':
            *filenames = (char *)malloc(strlen(optarg) + 1 * sizeof(char));
            if (filenames == NULL)
            {
                perror("Error: ");
                exit(-1);
            }
            strcpy(*filenames, optarg);
            f++;
            break;

        case 'm':
            *namesemaphore = (char *)malloc(strlen(optarg) + 1 * sizeof(char));
            if (namesemaphore == NULL)
            {
                perror("Error: ");
                exit(-1);
            }
            strcpy(*namesemaphore, optarg);
            m++;
            break;
        case '?':
            fprintf(stderr, "%s %c %s", "unknown option:", optopt, "\n");
            exit(EXIT_FAILURE);
            return -1;
        }
    }
    if (b != 0 || s != 0 || f != 0 || m != 0)
    {
        printf("invalid command line argument\n");
        exit(EXIT_FAILURE);
    }
    return 0;
}