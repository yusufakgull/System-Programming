#include "helper.h"
#include "myqueue.h"
#include "client.h"

int main(int argc, char *argv[])
{

    if (9 != argc)
    {
        puts("Unknown command line argument!");
        exit(EXIT_FAILURE);
    }
    char ipAddr[IP_LEN];
    int port;
    int id;
    char queryFilePath[PATH_LINE];
    char optC;
    while (-1 != (optC = getopt(argc, argv, ":a:p:i:o:")))
    {
        switch (optC)
        {
        case 'a':
            strcpy(ipAddr, optarg);
            break;
        case 'p':
            port = atoi(optarg);
            break;
        case 'i':
            id = atoi(optarg);
            break;
        case 'o':
            strcpy(queryFilePath, optarg);
            break;
        case ':':
        default:
            return 1;
            break;
        }
    }
    FILE *fp = fopen(queryFilePath, "r");
    if (fp == NULL)
    {
        perror("file open: ");
        exit(EXIT_FAILURE);
    }
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == sockfd)
    {
        puts("Error sockfd");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = inet_addr(ipAddr);
    memset(&(serverAddr.sin_zero), '\0', 8);
    connectingInfo(ipAddr, port, id);
    if (inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    if (connect(sockfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }
    fclose(fp);
    sendQuery(sockfd, id, queryFilePath);
    return 0;
}

void sendQuery(int sockfd, int id, char *queryFilePath)
{
    FILE *fp = fopen(queryFilePath, "r");
    if (fp == NULL)
    {
        perror("file open: ");
        exit(EXIT_FAILURE);
    }
    int queryCount = 0;
    char line[LINE_LEN];
    char *token;
    char *rest;
    char buffer[LINE_LEN] = {0};
    int rowSize;
    // int valread;
    struct Timer timer;

    while (fgets(line, sizeof(line), fp) != NULL)
    {
        rest = line;
        token = strtok_r(rest, " ", &rest);
        if (atoi(token) == id)
        {
            queryCount++;
            connectedInfo(id, rest);
            startTimer(&timer);
            send(sockfd, rest, strlen(rest), 0);
            read(sockfd, buffer, LINE_LEN);
            stopTimer(&timer);
            rowSize = atoi(buffer);
            printf("Server's response to Client-%d is %d records, and arrived in %.2f seconds\n", id, rowSize - 1, readTimer(&timer));
            // valread = read(sockfd, buffer, LINE_LEN);
            read(sockfd, buffer, LINE_LEN);
            printf("    %s\n", buffer);
            for (int i = 0; i < rowSize - 1; i++)
            {
                read(sockfd, buffer, LINE_LEN);
                // valread = read(sockfd, buffer, LINE_LEN);
                printf("%d   %s\n", i + 1, buffer);
            }
        }
    }
    printf("A total of %d queries were executed, client is terminating.\n", queryCount);
    fclose(fp);
}
void printwithdate(char *str)
{
    time_t tm = time(NULL);
    char timeStr[LINE_LEN];
    ctime_r(&tm, timeStr);
    timeStr[strlen(timeStr) - 1] = '\0';
    char line[2 * LINE_LEN];
    sprintf(line, "%s %s", timeStr, str);
    printf("%s", line);
}

void connectingInfo(char *ip, int port, int id)
{
    char line[LINE_LEN];
    sprintf(line, "Client-%d connecting to %s:%d\n", id, ip, port);
    printwithdate(line);
}

void connectedInfo(int id, char *query)
{
    char line[LINE_LEN];
    query[strlen(query) - 1] = '\0';
    sprintf(line, "Client-%d connected and sending query '%s'\n", id, query);
    printwithdate(line);
}