#include "func.h"
#include "myqueue.h"

int main(int argc, char *argv[])
{
    signal(SIGINT, sigHandler);
    properties inputs;
    if (fill_struct(&inputs, argc, argv))
        return 1;
    int max_size = 256;
    int rear = -1;
    int front = -1;
    int count = 0;
    char **qupath = malloc(max_size * sizeof(char *));
    scanFolder(&inputs, inputs.path, 1, &max_size, &count, &rear, &front, qupath);
    if (inputs.used.count == 0)
        printf("No file found\n");
    free(qupath);
    if (inputs.used.c_path == 1)
        free(inputs.path);
    if (inputs.used.c_fileName == 1)
        free(inputs.fileName);
    if (inputs.used.c_permission == 1)
        free(inputs.permission);
    return 0;
}
