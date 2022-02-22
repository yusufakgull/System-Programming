#include "func.h"

int main(int argc, char *argv[])
{
    signal(SIGINT, sigHandler);
    srand(time(NULL));
    int hotornot;
    char *namememory;
    char *filenames;
    char *namesemaphore;
    void *addr;
    int fd_r;
    int fd_w[256];
    char fd_wn[256][256];
    char fd_rn[256];

    input(argc, argv, &hotornot, &namememory, &filenames, &namesemaphore);
    sem_t *sem;
    sem = sem_open(namesemaphore, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR, 1);
    if (sem == SEM_FAILED)
    {
        perror("sem :");
        exit(-1);
    }

    if (!create_shared_mem(namememory, &addr))
        fill_shared_mem(addr); // if memory not exist then create
    if (hotornot > 0)
        insert_potato(hotornot, addr);

    createfifos(addr, filenames, &fd_r, fd_w, fd_rn, fd_wn, namesemaphore, sem);
    switch_potatos(fd_r, fd_w, addr, hotornot, fd_rn, fd_wn, sem);
    if (sem_close(sem) == -1)
    {
        exit(-1);
    }
    if (shm_unlink(namememory) == -1 && errno != ENOENT)
    {
        perror("unlink: ");
        exit(-1);
    }
    if (sem_unlink(namesemaphore) == -1 && errno != ENOENT)
    {
        perror("unlink: ");
        exit(-1);
    }
    free(namememory);
    free(filenames);
    free(namesemaphore);
    return 0;
}
