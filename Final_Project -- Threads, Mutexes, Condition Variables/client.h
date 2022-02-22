#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define IP_LEN 16
#define PATH_LINE 1024
void connectingInfo(char *ip, int port, int id);
void sendQuery(int sockfd, int id, char *queryFilePath);
void connectedInfo(int id, char *query);

#endif