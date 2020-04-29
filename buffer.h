#ifndef COMP421_LAB3_BUFFER_H
#define COMP421_LAB3_BUFFER_H

/** Struct for a Integer Buffer, meant for lists of free inodes and free blocks*/
struct buffer {
    /**
     * Size to go to until the buffer loops
     */
    int size;
    /**
     * Pointer to character array
     */
    int *b;
    /**
     * First filled space, next to be removed
     */
    int out;

    /**
     * First empty space, next to be filled
     */
    int in;
    /**
     * Condition on if the buffer is empty
     */
    int empty;
    /**
     * Condition on if the buffer is full
     */
    int full;
};

/**
 * Constructor message for a new buffer
 * @param size How much space to allocate for the buffer
 */
struct buffer *GetBuffer(int size);

/**
 * Adds a value to the buffers queue
 * @param buf Buffer to add to
 * @param c Character that is being added
 */
void PushToBuffer(struct buffer *buf, int i);

/**
 * Pops the next character from the buffer's queue
 * @param buf Buffer requesting character from
 * @return the character that was popped
 */
int PopFromBuffer(struct buffer *buf);
/**
 * Prints out the contents of the buffer
 * @param buf Buffer to print
 */
void PrintBuffer(struct buffer *buf);

#endif //COMP421_LAB3_BUFFER_H
