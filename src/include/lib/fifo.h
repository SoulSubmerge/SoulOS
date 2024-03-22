#ifndef SOUL_FIFO_H
#define SOUL_FIFO_H

#include <types/types.h>

typedef struct fifo_t
{
    char *buf;
    uint32 length;
    uint32 head;
    uint32 tail;
} fifo_t;

void fifoInit(fifo_t *fifo, char *buf, uint32 length);
bool fifoFull(fifo_t *fifo);
bool fifoEmpty(fifo_t *fifo);
char fifoGet(fifo_t *fifo);
void fifoPut(fifo_t *fifo, char byte);

#endif