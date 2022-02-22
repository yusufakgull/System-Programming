#include "myqueue.h"


void insertRear(char **path, char *item, int *pfront, int *prear, int *pcapacity, int *pcount)
{
    int i, k;

    if (*pfront == 0 && *prear == *pcapacity - 1)
    {
        (*pcapacity) *= 2;
        path = (char **)realloc(path, (*pcapacity) * sizeof(char *));
    }

    if (*pfront == -1)
    {
        *prear = *pfront = 0;
        path[*prear] = item;
        *pcount = 1;
    }
    else
    {
        if (*prear == *pcapacity - 1)
        {
            k = *pfront - 1;
            for (i = *pfront - 1; i < *prear; i++)
            {
                k = i;
                if (k == *pcapacity - 1)
                    path[k] = 0;
                else
                    path[k] = path[i + 1];
            }
            (*prear)--;
            (*pfront)--;
        }
        (*prear)++;
        path[*prear] = item;
        *pcount += 1;
    }
}

char *removeFront(char **path, int *pfront, int *prear, int *pcount)
{
    char *item;
    if (*pfront == -1)
    {
        fprintf(stderr, "%s", "\nDeque is empty.\n");
        return NULL;
    }
    item = path[*pfront];
    if (*pfront == *prear)
        *pfront = *prear = -1;
    else
        (*pfront)++;
    *pcount -= 1;
    return item;
}

char *removeRear(char **path, int *pfront, int *prear, int *pcount)
{
    char *item;
    if (*pfront == -1)
    {
        fprintf(stderr, "%s", "\nDeque is empty.\n");
        return NULL;
    }
    item = path[*prear];
    (*prear)--;
    if (*prear == -1)
        *pfront = -1;
    *pcount -= 1;
    return item;
}
