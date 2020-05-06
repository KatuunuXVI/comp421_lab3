#include "buffer.h"
#include <stdlib.h>
#include <stdio.h>

struct buffer *GetBuffer(int size) {
    struct buffer* newBuf = malloc(sizeof(struct buffer));
    newBuf->size = size;
    newBuf->b = malloc(sizeof(int) * size);
    newBuf->in = 0;
    newBuf->out = 0;
    /**
     * New buffer should ALWAYS be empty
     */
    newBuf->empty = 1;
    newBuf->full = 0;
    return newBuf;
}

void PushToBuffer(struct buffer *buf, int i) {
    if (buf->full) {
        return;
    }
    if (buf->empty) {
        buf->empty = 0;
    }
    buf->b[buf->in] = i;
    buf->in++;
    if (buf->in >= buf->size) {
        buf->in = 0;
    }
    if (buf->in == buf->out) {
        buf->full = 1;
    }
}

int PopFromBuffer(struct buffer *buf) {
    if (buf->empty) {
        return '\0';
    }
    int next = buf->b[buf->out];
    buf->out++;
    if (buf->out >= buf->size) {
        buf->out = 0;
    }
    if (buf->out == buf->in) {
        buf->empty = 1;
    }
    if (buf->full) {
        buf->full = 0;
    }
    return next;
}

void PrintBuffer(struct buffer *buf) {
    int i;
    if (buf->out < buf->in) {
        printf("[");
        for (i = buf->out; i < buf->in-1; i++) {
            printf("%d, ", buf->b[i]);
        }
        printf("%d]\n", buf->b[buf->in-1]);
    } else {
        printf("[");
        for (i = buf->out; i < buf->size; i++) {
            printf("%d, ", buf->b[i]);
        }
        for (i = 0; i < buf->in-1; i++) {
            printf("%d, ", buf->b[i]);
        }
        printf("%d]\n", buf->b[buf->in-1]);
    }

}
