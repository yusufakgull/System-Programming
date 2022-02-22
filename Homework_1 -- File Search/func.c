#include "func.h"
#include "myqueue.h"

int volatile still = 1;

void sigHandler(int csignal)
{
    still = 0;
    fprintf(stdout, "%s", "Stopped by signal `SIGINT' and all resources to return the system\n");
}

int fill_struct(properties *inputs, int argc, char *argv[])
{
    int opt;
    inputs->used.c_byt = 0;
    inputs->used.c_fileName = 0;
    inputs->used.c_fileType = 0;
    inputs->used.c_numberOfLinks = 0;
    inputs->used.c_path = 0;
    inputs->used.c_permission = 0;
    inputs->used.count = 0;
    int control = 0;
    while ((opt = getopt(argc, argv, "f:b:t:p:l:w:")) != -1)
    {
        switch (opt)
        {
        case 'w':
            inputs->path = (char *)malloc(strlen(optarg) + 1 * sizeof(char));
            if (inputs->path == NULL)
            {
                perror("Error: ");
                exit(1);
            }
            strcpy(inputs->path, optarg);
            inputs->used.c_path = 1;
            break;
        case 'f':
            inputs->fileName = (char *)malloc(strlen(optarg) + 1 * sizeof(char));
            if (inputs->fileName == NULL)
            {
                perror("Error: ");
                exit(1);
            }
            strcpy(inputs->fileName, optarg);
            inputs->used.c_fileName = 1;
            break;
        case 'b':
            inputs->byte = atoi(optarg);
            inputs->used.c_byt = 1;
            break;

        case 't':
            inputs->fileType = *optarg;
            inputs->used.c_fileType = 1;
            break;
        case 'p':
            inputs->permission = (char *)malloc(strlen(optarg) + 1 * sizeof(char));
            if (inputs->permission == NULL)
            {
                perror("Error: ");
                exit(1);
            }
            strcpy(inputs->permission, optarg);
            inputs->used.c_permission = 1;
            break;
        case 'l':
            inputs->numberOfLinks = atoi(optarg);
            inputs->used.c_numberOfLinks = 1;
            break;
        case '?':
            fprintf(stderr, "%s %c %s", "unknown option:", optopt, "\n");
            return 1;
        }
    }
    if (inputs->used.c_path == 0)
    {
        fprintf(stderr, "%s", "-w (path) is mandatory parameter\n");
        return 1;
    }

    control += inputs->used.c_byt;
    control += inputs->used.c_fileName;
    control += inputs->used.c_fileType;
    control += inputs->used.c_numberOfLinks;
    control += inputs->used.c_permission;
    if (control == 0)
    {
        fprintf(stderr, "%s", "At least one criteria must be employed (-f, -b, -t, -p, -l)\n");
        return 1;
    }

    return 0;
}

void scanFolder(properties *inputs, char *path, int depth, int *maxSize, int *count, int *rear, int *front, char **qupath)
{
    DIR *dir;
    struct dirent *dirp;
    struct stat buff;
    dir = opendir(path);
    if (dir == NULL)
    {
        fprintf(stderr, "%s %s", "Cannot open file.\n", path);
        return;
    }
    chdir(path);
    while ((dirp = readdir(dir)) != NULL && still)
    {

        if (((dirp->d_type) == DT_DIR) && ((strcmp(dirp->d_name, ".") == 0) || (strcmp(dirp->d_name, "..")) == 0))
            continue;

        if (stat(dirp->d_name, &buff) == 0)
        {

            if (checkIfSatisfy(*inputs, buff, dirp->d_name) && still)
            {
                if (inputs->used.count == 0)
                    printf("%s\n", inputs->path);
                while (*count != 0)
                {
                    printf("|");
                    for (int i = 0; i < depth - *count; i++)
                    {
                        printf("--");
                    }
                    printf("%s\n", removeFront(qupath, front, rear, count));
                }
                printf("|");
                for (int i = 0; i < depth; i++)
                {
                    printf("--");
                }
                printf("%s\n", dirp->d_name);
                inputs->used.count += 1;
            }
            if ((dirp->d_type) == DT_DIR && still)
            {
                insertRear(qupath, dirp->d_name, front, rear, maxSize, count);
                scanFolder(inputs, dirp->d_name, depth + 1, maxSize, count, rear, front, qupath);
            }
        }
    }

    if (*count != 0)
        removeRear(qupath, front, rear, count);
    chdir("..");
    closedir(dir);
}

char *permissions(mode_t pMode)
{
    char *per = (char *)malloc(10 * sizeof(char));
    if (per == NULL)
    {
        perror("Error: ");
        exit(1);
    }
    strcpy(per, (pMode & S_IRUSR) ? "r" : "-");
    strcat(per, (pMode & S_IWUSR) ? "w" : "-");
    strcat(per, (pMode & S_IXUSR) ? "x" : "-");
    strcat(per, (pMode & S_IRGRP) ? "r" : "-");
    strcat(per, (pMode & S_IWGRP) ? "w" : "-");
    strcat(per, (pMode & S_IXGRP) ? "x" : "-");
    strcat(per, (pMode & S_IROTH) ? "r" : "-");
    strcat(per, (pMode & S_IWOTH) ? "w" : "-");
    strcat(per, (pMode & S_IXOTH) ? "x" : "-");
    return per;
}

char fileType(mode_t iMode)
{
    if (S_ISREG(iMode))
        return 'f';
    else if (S_ISDIR(iMode))
        return 'd';
    else if (S_ISBLK(iMode))
        return 'b';
    else if (S_ISCHR(iMode))
        return 'c';
    else if (S_ISSOCK(iMode))
        return 's';
    else if (S_ISFIFO(iMode))
        return 'p';
    else if (S_ISLNK(iMode))
        return 'l';
    return '\0';
}

char tolowerr(char ch)
{
    if (ch >= 'A' && ch <= 'Z')
        ch = 'a' + (ch - 'A');
    return ch;
}

int checkNameWithRegex(char *input, char *dirName)
{
    char *ip = input;
    char *dp = dirName;
    char temp;
    int i = 1;
    if (*ip != '+')
    {
        while (*ip != '\0' && *dp != '\0')
        {
            if (*ip != '+')
            {
                if (tolowerr(*ip) != tolowerr(*dp))
                {
                    return 1;
                }
                else
                {
                    ip++;
                    dp++;
                }
            }
            else
            {

                temp = tolowerr(*(ip - 1));
                while (temp == *dp)
                {
                    if (tolowerr(*(ip + i)) == temp)
                    {
                        i++;
                    }
                    dp++;
                }
                if (i == 1)
                    ip++;
                else
                    ip = ip + i;
            }
            if ((*ip == '\0' && *dp != '\0') || ((*ip != '\0' && *ip != '+') && *dp == '\0'))
                return 1;
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

int checkIfSatisfy(properties inputs, struct stat buff, char *dirName)
{
    if (inputs.used.c_fileName == 1)
        if (checkNameWithRegex(inputs.fileName, dirName) == 1)
            return 0;

    if (inputs.used.c_byt == 1)
        if (buff.st_size != inputs.byte)
            return 0;

    if (inputs.used.c_fileType == 1)
        if (fileType(buff.st_mode) != inputs.fileType)
            return 0;

    if (inputs.used.c_permission == 1)
    {
        char *temp = permissions(buff.st_mode);
        if (strcmp(temp, inputs.permission))
        {
            free(temp);
            return 0;
        }
        
        free(temp);
    }

    if (inputs.used.c_numberOfLinks == 1)
        if (inputs.numberOfLinks != buff.st_nlink)
            return 0;

    return 1;
}