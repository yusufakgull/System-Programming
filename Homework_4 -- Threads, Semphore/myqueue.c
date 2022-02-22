#include "myqueue.h"

char items[SIZE];
int front = -1, rear = -1;
void insertToQueue(const char value)
{

    if (front == -1)
    {
        rear = 0;
        front = 0;
        items[rear] = value;
    }
    else if (rear == SIZE - 1 && front != 0)
    {
        rear = 0;
        items[rear] = value;
    }
    else
    {
        rear++;
        items[rear] = value;
    }
}

char deleteFromQueue()
{
    char c = items[front];
    if (front == rear)
    {
        front = -1;
        rear = -1;
    }
    else if (front == SIZE - 1)
    {
        front = 0;
    }
    else
    {
        front++;
    }
    return c;
}