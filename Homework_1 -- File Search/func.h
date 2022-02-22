#ifndef FUNC_H_
#define FUNC_H_

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

typedef struct Properties_check
{
    int c_path;
    int c_fileName;
    int c_byt;
    int c_fileType;
    int c_permission;
    int c_numberOfLinks;
    int count;

} properties_check;

typedef struct Properties
{
    char *path;
    char *fileName;
    int byte;
    char fileType;
    char *permission;
    int numberOfLinks;
    properties_check used;

} properties;

int fill_struct(properties *inputs, int argc, char *argv[]);
void scanFolder(properties *inputs, char *path, int depth, int *maxSize, int *count, int *rear, int *front, char **qupath);
char *permissions(mode_t pMode);
void printType(mode_t iMode);
int checkIfSatisfy(properties inputs, struct stat buff, char *dirName);
char tolowerr(char ch);
void sigHandler(int csignal);

#endif