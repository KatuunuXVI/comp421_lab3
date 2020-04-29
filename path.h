#include <comp421/filesystem.h>

typedef struct PathIterator {
    struct PathIterator *head;
    struct PathIterator *next;
    char data[DIRNAMELEN];
} PathIterator;

/*
 * Given pathname as Yalnix argument, parse it
 * into a linked list of component.
 */
PathIterator *ParsePath(char *pathname);

/*
 * Free pathIterator linked list after finished using it.
 * You must provided the head of the linked list.
 */
int DeletePathIterator(PathIterator *it);
