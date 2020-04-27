#include <comp421/filesystem.h>

struct PathIterator {
    struct PathIterator *head;
    struct PathIterator *next;
    char data[DIRNAMELEN];
};

struct PathIterator *ParsePath(char *pathname);

int DeletePathIterator(struct PathIterator *it);

void TestPathIterator();
