#include <lib/fifo.h>
#include <kernel/assert.h>

static inline uint32 fifoNext(fifo_t *fifo, uint32 pos)
{
    return (pos + 1) % fifo->length;
}

void fifoInit(fifo_t *fifo, char *buf, uint32 length)
{
    fifo->buf = buf;
    fifo->length = length;
    fifo->head = 0;
    fifo->tail = 0;
}

bool fifoFull(fifo_t *fifo)
{
    bool full = (fifoNext(fifo, fifo->head) == fifo->tail);
    return full;
}

bool fifoEmpty(fifo_t *fifo)
{
    return (fifo->head == fifo->tail);
}

char fifoGet(fifo_t *fifo)
{
    assert(!fifoEmpty(fifo), "The expected fifo is not empty.");
    char byte = fifo->buf[fifo->tail];
    fifo->tail = fifoNext(fifo, fifo->tail);
    return byte;
}

void fifoPut(fifo_t *fifo, char byte)
{
    while (fifoFull(fifo))
    {
        fifoGet(fifo);
    }
    fifo->buf[fifo->head] = byte;
    fifo->head = fifoNext(fifo, fifo->head);
}