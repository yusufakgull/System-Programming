#include "myqueue.h"

int items[queueSize];
int front = -1, rear = -1;
int size = 0;
void insertToQueue(const int value)
{

    if (size == queueSize)
        return;
    if (front == -1)
    {
        rear = 0;
        front = 0;
        items[rear] = value;
    }
    else if (rear == queueSize - 1 && front != 0)
    {
        rear = 0;
        items[rear] = value;
    }
    else
    {
        rear++;
        items[rear] = value;
    }
    size++;
}

int deleteFromQueue()
{
    int c = items[front];
    if (front == rear)
    {
        front = -1;
        rear = -1;
    }
    else if (front == queueSize - 1)
    {
        front = 0;
    }
    else
    {
        front++;
    }
    size--;
    return c;
}

int getQueueSize()
{
    return size;
}