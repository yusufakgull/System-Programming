
#include "server.h"
#include "helper.h"
#include "myqueue.h"

serverConfig configs;
DataBase database;

int main(int argc, char *argv[])
{
    readCommandLine(argc, argv);
    configs.lockFD = blockInstance(LOCK_PATH);
    become_daemon();
    configs.lockFD = blockInstance(LOCK_PATH);
    configs.logFD = open(configs.logFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    char line[LINE_LEN * 3];
    sprintf(line, "Executing with parameters:\n  \t\t\t-p %d\n    \t\t\t-o %s\n    \t\t\t-l %d\n    \t\t\t-d %s\n", configs.port, configs.logFilePath, configs.poolSize, configs.datasetPath);
    serverLog(line);
    struct sigaction sigIntHandler;
    sigIntHandler.sa_handler = sigHandler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);
    loadDataSet();

    struct sockaddr_in clientAddr = {0};
    socklen_t structSize = sizeof(clientAddr);
    busy = configs.poolSize;
    initThreads();
    prepareSocket();
    int newsockfd;
    while (still)
    {
        newsockfd = accept(configs.sockfd, (struct sockaddr *)&clientAddr, &structSize);
        if (newsockfd == -1 && still)
        {
            perror("new socket : ");
            exit(EXIT_FAILURE);
        }
        if (0 != pthread_mutex_lock(&configs.qm))
        {
            perror("mutex lock main: ");
            exit(EXIT_FAILURE);
        }
        if (busy == configs.poolSize)
        {
            serverLog("No thread is available! Waiting...\n");
        }
        insertToQueue(newsockfd);
        if (0 != pthread_cond_broadcast(&configs.qcond))
        {
            perror("main, thread cond");
            exit(EXIT_FAILURE);
        }
        if (0 != pthread_mutex_unlock(&configs.qm))
        {
            perror("unlock mutex : ");
            exit(EXIT_FAILURE);
        }
    }
    freeAll();
    exit(EXIT_SUCCESS);
}

int become_daemon()
{
    int maxfd, fd;
    switch (fork())
    {
    case -1:
        return -1;
    case 0:
        break;
    default:
        exit(EXIT_SUCCESS);
    }
    if (setsid() == -1)
        return -1;
    switch (fork())
    {
    case -1:
        return -1;
    case 0:
        break;
    default:
        exit(EXIT_SUCCESS);
    }
    umask(0);

    maxfd = sysconf(_SC_OPEN_MAX);
    if (maxfd == -1)
        maxfd = 8192;
    for (fd = 0; fd < maxfd; fd++)
        close(fd);

    close(STDIN_FILENO); /* Reopen standard fd's to /dev/null */
    fd = open("/dev/null", O_RDWR);
    if (fd != STDIN_FILENO) /* 'fd' should be 0 */
        return -1;
    if (dup2(STDIN_FILENO, STDOUT_FILENO) != STDOUT_FILENO)
        return -1;
    if (dup2(STDIN_FILENO, STDERR_FILENO) != STDERR_FILENO)
        return -1;
    return 0;
}

int blockInstance(char *filename)
{
    int fd;
    fd = open(filename, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (-1 == fd)
    {
        perror("lockInstance, open");
        exit(EXIT_FAILURE);
    }
    if (-1 == fcntl(fd, F_SETFD, FD_CLOEXEC))
    {
        perror("lockInstance");
        exit(EXIT_FAILURE);
    }
    struct flock lock;
    memset(&lock, '\0', sizeof(lock));
    lock.l_type = F_WRLCK;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    if (-1 == fcntl(fd, F_SETLK, &lock))
    {
        if (EAGAIN == errno || EACCES == errno)
        {
            printf("An instance is already running!\n");
            exit(EXIT_FAILURE);
        }
        else
        {
            perror("lockInstance, fcntl");
            exit(EXIT_FAILURE);
        }
    }
    if (-1 == ftruncate(fd, 0))
    {

        perror("lockInstance, ftruncate");
        exit(EXIT_FAILURE);
    }
    char buf[LINE_LEN];
    sprintf(buf, "%ld\n", (long)getpid());
    if (strlen(buf) != write(fd, buf, strlen(buf)))
    {
        perror("lockInstance");
        exit(EXIT_FAILURE);
    }
    return fd;
}
void sigHandler(int csignal)
{
    still = 0;
}
void initThreads()
{

    char line[LINE_LEN];
    configs.threads = (pthread_t *)calloc(configs.poolSize, sizeof(pthread_t));
    if (0 != pthread_cond_init(&configs.qcond, NULL))
    {
        perror("init threads");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_cond_init(&configs.okToRead, NULL))
    {
        perror("init threads");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_cond_init(&configs.okToWrite, NULL))
    {
        perror("init threads");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_mutex_init(&configs.m, NULL))
    {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_mutex_init(&configs.qm, NULL))
    {
        perror("pthread_mutex_init");
        exit(EXIT_FAILURE);
    }
    configs.AR = 0;
    configs.AW = 0;
    configs.WR = 0;
    configs.WW = 0;
    sprintf(line, "A pool of %d threads has been created\n", configs.poolSize);
    serverLog(line);
    for (int i = 0; i < configs.poolSize; ++i)
    {
        if (0 != pthread_create(&configs.threads[i], NULL, worker, (void *)(intptr_t)i))
        {
            perror("createWorker, pthread_create");
            exit(EXIT_FAILURE);
        }
    }
}

void *worker(void *data)
{
    int byte;
    int id = (intptr_t)data;
    char buffer[4096] = {0};
    int newsockfd;
    int count;
    query temp;
    queryRespond tempRespond;
    char line[LINE_LEN * 2];
    while (still)
    {
        sprintf(line, "Thread #%d: waiting for connection\n", id);
        serverLog(line);
        pthread_mutex_lock(&configs.qm);
        busy--;
        while (getQueueSize() == 0 && still)
            pthread_cond_wait(&configs.qcond, &configs.qm);
        if (still)
        {
            busy++;
            newsockfd = deleteFromQueue();
            pthread_mutex_unlock(&configs.qm);
            sprintf(line, "A connection has been delegated to thread id #%d\n", id);
            serverLog(line);
            count = 0;
            byte = 1;
            while (byte != 0 && still)
            {
                byte = read(newsockfd, buffer, 4096);

                if (byte != 0 && still)
                {
                    sprintf(line, "Thread #%d: received query %s\n", id, buffer);
                    serverLog(line);

                    if (buffer[0] == 'S' || buffer[0] == 's') //READER
                    {
                        pthread_mutex_lock(&configs.m);
                        while ((configs.AW + configs.WW) > 0)
                        {
                            configs.WR++;
                            pthread_cond_wait(&configs.okToRead, &configs.m);
                            configs.WR--;
                        }
                        configs.AR++;
                        pthread_mutex_unlock(&configs.m);
                        count++;
                        temp = createQuery(buffer);
                        tempRespond = getQuery(temp);
                        usleep(500000);
                        sendRespond(tempRespond, newsockfd);
                        pthread_mutex_lock(&configs.m);
                        configs.AR--;
                        if (configs.AR == 0 && configs.WW > 0)
                        {
                            pthread_cond_signal(&configs.okToWrite);
                        }
                        pthread_mutex_unlock(&configs.m);
                    }
                    else //writer
                    {
                        pthread_mutex_lock(&configs.m);
                        while ((configs.AW + configs.AR) > 0)
                        {
                            configs.WW++;
                            pthread_cond_wait(&configs.okToWrite, &configs.m);
                            configs.WW--;
                        }
                        configs.AW++;
                        pthread_mutex_unlock(&configs.m);
                        count++;
                        temp = createQuery(buffer);
                        // queryInfo(temp);
                        tempRespond = getQuery(temp);
                        usleep(500000);
                        sendRespond(tempRespond, newsockfd);
                        pthread_mutex_lock(&configs.m);
                        configs.AW--;
                        if (configs.WW > 0)
                        {
                            pthread_cond_signal(&configs.okToWrite);
                        }
                        else if (configs.WR > 0)
                        {
                            if (0 != pthread_cond_broadcast(&configs.okToRead))
                            {
                                perror("broadcast error");
                                exit(EXIT_FAILURE);
                            }
                        }
                        pthread_mutex_unlock(&configs.m);
                    }
                    freeQuery(temp);
                    freeQueryRespond(tempRespond);
                    sprintf(line, "Thread #%d: query completed, %d records have been returned.\n", id, tempRespond.rowSize - 1);
                    serverLog(line);
                }
            }
            if (-1 == close(newsockfd))
            {
                perror("worker loop, close");
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (0 != pthread_cond_broadcast(&configs.qcond))
            {
                perror("main, thread cond");
                exit(EXIT_FAILURE);
            }
            pthread_mutex_unlock(&configs.qm);
        }
    }
    return NULL;
}

void prepareSocket()
{

    configs.sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == configs.sockfd)
    {
        perror("error prepare");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(configs.port);
    sa.sin_addr.s_addr = INADDR_ANY;
    memset(&(sa.sin_zero), '\0', 8);
    if (-1 == bind(configs.sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr)))
    {
        perror("error prepare socket.");
        exit(EXIT_FAILURE);
    }
    if (-1 == listen(configs.sockfd, SOMAXCONN))
    {
        perror("error LISTEN socket.");
        exit(EXIT_FAILURE);
    }
}
query createQuery(char *input)
{
    query temp;
    temp.columnSize = 0;

    tostrlower(input);

    char *rest = input;
    char *token = strtok_r(rest, " ", &rest);
    char *tempstr;
    char *tofree;
    char *tofree2;
    if (!strcmp("select", token))
    {
        tofree = tempstr = strdup(rest);
        token = strtok_r(rest, " ", &rest);
        if (!strcmp("distinct", token))
        {
            temp.queryType = SELECT_DISTINCT;
            free(tofree);
            tofree = tempstr = strdup(rest);
        }
        else
        {
            temp.queryType = SELECT;
            tofree2 = rest = strdup(tempstr);
        }
        while ((token = strtok_r(rest, ", ", &rest)))
        {
            if (!strcmp(token, "from"))
                break;
            temp.columnSize++;
        }
        if (temp.queryType == SELECT)
            free(tofree2);
        temp.columnIndex = (int *)malloc(temp.columnSize * sizeof(int));

        node *tmpnode = database.head->next;

        for (int i = 0; i < temp.columnSize; i++)
        {
            token = strtok_r(tempstr, ", ", &tempstr);
            if (!strcmp(token, "*"))
            {
                temp.columnIndex[i] = -1;
                break;
            }
            if (!strcmp(token, "from"))
                break;
            bool checkexist = false;
            for (int j = 0; j < database.columnSize; j++)
            {
                if (strcmp(token, tmpnode->row[j]) == 0)
                {
                    temp.columnIndex[i] = j;
                    checkexist = true;
                    break;
                }
            }
            if (!checkexist)
            {
                puts("Unknown column name!!");
                exit(EXIT_FAILURE);
            }
        }
        free(tofree);
    }
    else if (!strcmp("update", token))
    {
        temp.queryType = UPDATE;
        token = strtok_r(rest, ", ", &rest);
        tofree = tempstr = strdup(rest);

        tofree2 = rest = strdup(tempstr);
        token = strtok_r(rest, " ", &rest);

        while ((token = strtok_r(rest, ", ", &rest)))
        {
            if (!strcmp(token, "where"))
                break;
            temp.columnSize++;
        }
        free(tofree2);
        temp.columnIndex = (int *)malloc(temp.columnSize * sizeof(int));
        temp.updatevalue = malloc(temp.columnSize * sizeof(char *));

        node *tmpnode = database.head->next;

        token = strtok_r(tempstr, ", ", &tempstr);
        char *token2;
        for (int i = 0; i < temp.columnSize; i++)
        {
            token = strtok_r(tempstr, ", ", &tempstr);
            if (!strcmp(token, "where"))
                break;
            token2 = strtok(token, "='");
            for (int j = 0; j < database.columnSize; j++)
            {
                if (strcmp(token2, tmpnode->row[j]) == 0)
                {
                    temp.columnIndex[i] = j;
                    break;
                }
            }
            token2 = strtok(NULL, "='");
            temp.updatevalue[i] = malloc((strlen(token2) + 1) * sizeof(char));
            strcpy(temp.updatevalue[i], token2);
        }
        token = strtok_r(tempstr, " ='", &tempstr);
        puts(tempstr);
        token = strtok_r(tempstr, " ='", &tempstr);
        puts(tempstr);
        for (int j = 0; j < database.columnSize; j++)
        {
            if (strcmp(token, tmpnode->row[j]) == 0)
            {
                temp.updateWhereIndex = j;
                break;
            }
        }
        token = strtok_r(tempstr, "='", &tempstr);
        temp.selectvalue = malloc((strlen(token) + 1) * sizeof(char));
        strcpy(temp.selectvalue, token);
        free(tofree);
    }
    else
    {
        puts("Unknown query format!!\n");
        exit(EXIT_FAILURE);
    }
    return temp;
}

queryRespond getQuery(query query_)
{

    queryRespond temp;
    node *tempNode = database.head->next;
    int respondCount = 0;
    bool control_distinct = true;
    int sameCount = 0;
    if (query_.queryType == SELECT)
    {
        temp.rowSize = database.rowSize;
        temp.respondtype = SELECT;
        if (query_.columnSize == 1 && query_.columnIndex[0] == -1)
        {
            temp.columnIndex = calloc(database.columnSize, sizeof(int *));
            temp.columnSize = database.columnSize;
            for (int i = 0; i < database.columnSize; i++)
            {
                temp.columnIndex[i] = i;
            }
            return temp;
        }
        else
        {
            temp.columnSize = query_.columnSize;
            temp.columnIndex = calloc(query_.columnSize, sizeof(int *));
            for (int i = 0; i < temp.columnSize; i++)
            {
                temp.columnIndex[i] = query_.columnIndex[i];
            }
        }
    }
    else if (query_.queryType == SELECT_DISTINCT)
    {

        temp.respondtype = SELECT_DISTINCT;
        temp.rowSize = 0;

        temp.rowIndex = calloc(database.rowSize, sizeof(node *));

        if (query_.columnSize == 1 && query_.columnIndex[0] == -1)
        {
            temp.columnIndex = calloc(database.columnSize, sizeof(int *));
            temp.columnSize = database.columnSize;
            for (int i = 0; i < database.columnSize; i++)
            {
                temp.columnIndex[i] = i;
            }
        }
        else
        {
            temp.columnSize = query_.columnSize;
            temp.columnIndex = calloc(temp.columnSize, sizeof(int *));
            for (int i = 0; i < temp.columnSize; i++)
            {
                temp.columnIndex[i] = query_.columnIndex[i];
            }
        }
        for (int i = 0; i < database.rowSize; i++)
        {
            for (int k = 0; k < respondCount && control_distinct; k++)
            {

                for (int l = 0; l < temp.columnSize; l++)
                {
                    if (!strcmp(temp.rowIndex[k]->row[temp.columnIndex[l]], tempNode->row[temp.columnIndex[l]]))
                        sameCount++;
                    else
                        break;
                }
                if (sameCount == temp.columnSize)
                {
                    control_distinct = false;
                }
                sameCount = 0;
            }

            if (control_distinct)
            {
                respondCount++;
                temp.rowSize++;
                temp.rowIndex[respondCount - 1] = tempNode;
            }
            else
            {
                control_distinct = true;
            }

            tempNode = tempNode->next;
        }
    }
    else if (query_.queryType == UPDATE)
    {

        temp.respondtype = UPDATE;
        temp.rowSize = 0;
        temp.rowIndex = calloc(database.rowSize, sizeof(node *));
        temp.columnSize = query_.columnSize;
        temp.columnIndex = calloc(temp.columnSize, sizeof(int *));
        for (int i = 0; i < temp.columnSize; i++)
        {
            temp.columnIndex[i] = query_.columnIndex[i];
        }
        temp.rowSize++;
        temp.rowIndex[temp.rowSize - 1] = tempNode;
        for (int i = 0; i < database.rowSize; i++)
        {
            if (!strcmp(tempNode->row[query_.updateWhereIndex], query_.selectvalue))
            {
                temp.rowSize++;
                for (int j = 0; j < query_.columnSize; j++)
                {
                    free(tempNode->row[query_.columnIndex[j]]);
                    tempNode->row[query_.columnIndex[j]] = strdup(query_.updatevalue[j]);
                    temp.rowIndex[temp.rowSize - 1] = tempNode;
                }
            }
            tempNode = tempNode->next;
        }
    }
    else
    {
        puts("Unkonwn query, terminating.");
        exit(EXIT_FAILURE);
    }
    return temp;
}

void sendRespond(queryRespond respond, int sockfd)
{
    char line[LINE_LEN];
    char *ptr = line;
    node *temp;
    if (respond.respondtype == SELECT)
    {
        temp = database.head->next;
        sprintf(ptr, "%d", database.rowSize);
        send(sockfd, line, LINE_LEN, 0);
        for (int i = 0; i < database.rowSize; i++)
        {
            line[0] = '\0';
            ptr = line;
            for (int j = 0; j < respond.columnSize; j++)
            {
                ptr += sprintf(ptr, "%s\t", temp->row[respond.columnIndex[j]]);
            }
            send(sockfd, line, LINE_LEN, 0);
            temp = temp->next;
        }
    }
    else if (respond.respondtype == SELECT_DISTINCT || respond.respondtype == UPDATE)
    {

        sprintf(ptr, "%d", respond.rowSize);
        send(sockfd, line, LINE_LEN, 0);
        line[0] = '\0';
        for (int i = 0; i < respond.rowSize; i++)
        {
            temp = respond.rowIndex[i];
            line[0] = '\0';
            ptr = line;
            for (int j = 0; j < respond.columnSize; j++)
            {
                ptr += sprintf(ptr, "%s,", temp->row[respond.columnIndex[j]]);
            }
            send(sockfd, line, LINE_LEN, 0);
            temp = temp->next;
        }
    }
}

void serverLog(char *str)
{
    if (-1 == configs.logFD)
    {
        return;
    }
    time_t tm = time(NULL);
    char timeStr[LINE_LEN];
    ctime_r(&tm, timeStr);
    timeStr[strlen(timeStr) - 1] = '\0';
    char line[2 * LINE_LEN];
    sprintf(line, "%s %s", timeStr, str);

    if (-1 == write(configs.logFD, line, strlen(line)))
    {
        perror("server log write");
        exit(EXIT_FAILURE);
    }
}
void loadDataSet()
{

    serverLog("Loading dataset...\n");
    struct Timer timer;
    startTimer(&timer);
    FILE *fp = fopen(configs.datasetPath, "r");
    if (fp == NULL)
    {
        perror("file open: ");
        exit(EXIT_FAILURE);
    }
    char line[LINE_LEN];

    fgets(line, sizeof(line), fp);
    database.columnSize = findColumnCount(line);
    database.rowSize = 0;
    fseek(fp, 0, SEEK_SET); // return fp to begin

    database.head = malloc(sizeof(node));
    node *temp = database.head;
    char *token, *str1, *tofree;
    int length;
    while (fgets(line, sizeof(line), fp) != NULL && still)
    {
        length = strlen(line);
        if (line[length - 1] == '\n')
            line[--length] = '\0';
        if (line[length - 1] == '\r')
            line[--length] = '\0';

        tofree = str1 = strdup(line);
        temp->next = malloc(sizeof(node));
        if (temp->next == NULL)
        {
            perror("Malloc server: ");
            exit(EXIT_FAILURE);
        }
        temp = temp->next;
        temp->row = malloc(database.columnSize * sizeof(char *));
        if (temp->row == NULL)
        {
            perror("Malloc server: ");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < database.columnSize; i++)
        {
            if ('"' == (char)str1[0])
            {
                str1++;
                token = strsep(&str1, "\"");
                str1++;
            }
            else
            {
                token = strsep(&str1, ",");
            }
            temp->row[i] = malloc((strlen(token) + 1) * sizeof(char));
            if (temp->row[i] == NULL)
            {
                perror("Malloc server: ");
                exit(EXIT_FAILURE);
            }
            tostrlower(token);
            strcpy(temp->row[i], token);
        }
        database.rowSize++;
        free(tofree);
        temp->next = NULL;
    }
    stopTimer(&timer);
    if (still)
    {
        char line2[LINE_LEN];
        sprintf(line2, "Dataset loaded in %.2f seconds with %d records.\n", readTimer(&timer), database.rowSize);
        serverLog(line2);
    }
    fclose(fp);
}

void freeAll()
{

    serverLog("Termination signal received, waiting for ongoing threads to complete.\n");

    free_threads();

    node *temp = database.head->next;
    free(database.head);
    node *backNode;
    while (temp != NULL)
    {
        backNode = temp;
        temp = temp->next;
        for (int i = 0; i < database.columnSize; i++)
        {
            free(backNode->row[i]);
        }
        free(backNode->row);
        free(backNode);
    }
    database.head = NULL;
    serverLog("All threads have terminated, server shutting down.\n");
}

void free_threads()
{

    for (int i = 0; i < configs.poolSize; i++)
    {
        if (0 != pthread_join(configs.threads[i], NULL))
        {
            perror("freethreads, pthread_join");
            exit(EXIT_FAILURE);
        }
    }
    if (0 != pthread_mutex_destroy(&configs.m))
    {
        perror("freethreads, pthread_mutex_destroy");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_mutex_destroy(&configs.qm))
    {
        perror("freethreads, pthread_mutex_destroy");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_cond_destroy(&configs.okToRead))
    {
        perror("free threads, pthread_cond_destroy");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_cond_destroy(&configs.okToWrite))
    {
        perror("free threads, pthread_cond_destroy");
        exit(EXIT_FAILURE);
    }
    if (0 != pthread_cond_destroy(&configs.qcond))
    {
        perror("free threads, pthread_cond_destroy");
        exit(EXIT_FAILURE);
    }
    free(configs.threads);
    if (-1 == close(configs.lockFD))
    {
        perror("close");
        exit(EXIT_FAILURE);
    }
}
void readCommandLine(int argc, char **argv)
{
    if (argc != 9)
    {
        puts("Missing or extra command line argument!!");
        exit(EXIT_FAILURE);
    }
    char opt;
    while ((opt = getopt(argc, argv, "p:o:l:d:")) != -1)
    {
        switch (opt)
        {
        case 'p':
            configs.port = atoi(optarg);
            break;
        case 'o':
            strcpy(configs.logFilePath, optarg);
            break;
        case 'l':
            configs.poolSize = atoi(optarg);
            if (atoi(optarg) < 2)
            {
                puts("Number of thread in the pool must be greater than 1.");
                exit(EXIT_FAILURE);
            }
            break;
        case 'd':
            strcpy(configs.datasetPath, optarg);
            break;
        case '?':
            fprintf(stderr, "%s %c %s", "unknown option:", optopt, "\n");
            exit(EXIT_FAILURE);
        }
    }
    // configs.logFD = open(configs.logFilePath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    FILE *fp = fopen(configs.datasetPath, "r");
    if (fp == NULL)
    {
        perror("file open: ");
        exit(EXIT_FAILURE);
    }
    fclose(fp);
}

void freeQueryRespond(queryRespond qr)
{
    free(qr.columnIndex);
    if (qr.respondtype != SELECT)
        free(qr.rowIndex);
}
void freeQuery(query query_)
{
    free(query_.columnIndex);
    if (query_.queryType == UPDATE)
    {
        free(query_.selectvalue);
        for (int i = 0; i < query_.columnSize; i++)
        {
            free(query_.updatevalue[i]);
        }
        free(query_.updatevalue);
    }
}
void queryInfo(query query_)
{

    switch (query_.queryType)
    {
    case SELECT:
        puts("query type = SELECT");
        break;
    case SELECT_DISTINCT:
        puts("query type = SELECT_DISTINCT");
        break;
    case UPDATE:
        puts("query type = UPDATE");
        break;
    default:
        break;
    }

    printf("Column size = %d\n", query_.columnSize);
    printf("Column index = ");
    for (int i = 0; i < query_.columnSize; i++)
    {
        printf("%d ", query_.columnIndex[i]);
    }
    puts("");
    if (query_.queryType == UPDATE)
    {
        printf("update where index : %d\n", query_.updateWhereIndex);
        printf("if value is %s\n", query_.selectvalue);
        printf("Update values = ");
        for (int i = 0; i < query_.columnSize; i++)
        {
            printf("%s ", query_.updatevalue[i]);
        }
        puts("");
    }
}