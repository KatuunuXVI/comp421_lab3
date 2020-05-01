#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "path.h"
#include "dirname.h"

/*
Arg: /abc/def/ghi.c
Result:
/ -> abc -> def -> ghi.c -> NULL

Arg: xyz/123-456
Result:

xyz -> 123-456 -> NULL

Arg: ../../foo/bar/baz/example.txt
Result:

.. -> .. -> foo -> bar -> baz -> example.txt -> NULL

Arg: silly/./././././././example.txt
Result:

silly -> . -> . -> ... -> example.txt -> NULL

Arg: //////dirname//subdirname///file
Result:

/ -> dirname -> subdirname -> file -> NULL

Arg: /a/b/c/
Result:

/ -> a -> b -> c -> . -> NULL
*/

/*
 * Private helper method which allocate PathIterator.
 */
PathIterator *CreatePathIterator(PathIterator *head) {
    PathIterator *it = malloc(sizeof(PathIterator));
    it->head = head;
    it->next = NULL;
    return it;
}

/*
 * Given pathname as Yalnix argument, parse it
 * into a linked list of component.
 */
PathIterator *ParsePath(char *pathname) {
    PathIterator *it = CreatePathIterator(NULL);
    PathIterator *head = it;

    int i = 0;
    int start = 0;
    int found = 0;
    char next;

    // Backslash is found in the root position
    if (pathname[0] == '/') {
        SetDirectoryName(it->data, pathname, 0, 1);
        it->next = CreatePathIterator(head);
        it = it->next;
        i++;
        start = 1;
        found = 1;
    }

    while ((next = pathname[i]) != '\0') {
        if (found == 0 && next == '/') {
            // Backslash is just found
            SetDirectoryName(it->data, pathname, start, i);
            it->next = CreatePathIterator(head);
            it = it->next;
            found = 1;
        } else if (found == 1 && next != '/') {
            // Backslash is just finished.
            found = 0;
            start = i;
        }
        i++;
    }

    if (found) {
        // If iterator is finished after backslash is found,
        // last component is current directory
        SetDirectoryName(it->data, ".", 0, 1);
        it->next = CreatePathIterator(head);
    } else {
        // Last remaining path
        SetDirectoryName(it->data, pathname, start, i);
        it->next = CreatePathIterator(head);
    }

    return head;
}

/*
 * Free pathIterator linked list after finished using it.
 * You must provided the head of the linked list.
 */
int DeletePathIterator(PathIterator *head) {
    PathIterator *prev;
    PathIterator *it = head;

    while (it != NULL) {
        prev = it;
        it = it->next;
        free(prev);
    }
    return 0;
}

void TestPathIterator() {
    PathIterator *it;

    printf("Testing /abc/def/ghi.c\n");
    it = ParsePath("/abc/def/ghi.c");
    assert(strcmp(it->data, "/") == 0);
    it = it->next;
    assert(strcmp(it->data, "abc") == 0);
    it = it->next;
    assert(strcmp(it->data, "def") == 0);
    it = it->next;
    assert(strcmp(it->data, "ghi.c") == 0);
    it = it->next;
    assert(it->next == NULL);
    DeletePathIterator(it->head);

    printf("Testing xyz/123-456\n");
    it = ParsePath("xyz/123-456");
    assert(strcmp(it->data, "xyz") == 0);
    it = it->next;
    assert(strcmp(it->data, "123-456") == 0);
    it = it->next;
    printf("it->data: %s\n", it->data);
    assert(it->next == NULL);
    DeletePathIterator(it->head);

    printf("Testing ../../foo/bar/baz/example.txt\n");
    it = ParsePath("../../foo/bar/baz/example.txt");
    assert(strcmp(it->data, "..") == 0);
    it = it->next;
    assert(strcmp(it->data, "..") == 0);
    it = it->next;
    assert(strcmp(it->data, "foo") == 0);
    it = it->next;
    assert(strcmp(it->data, "bar") == 0);
    it = it->next;
    assert(strcmp(it->data, "baz") == 0);
    it = it->next;
    assert(strcmp(it->data, "example.txt") == 0);
    it = it->next;
    assert(it->next == NULL);
    DeletePathIterator(it->head);

    printf("Testing silly/./././././././example.txt\n");
    it = ParsePath("silly/./././././././example.txt");
    assert(strcmp(it->data, "silly") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(strcmp(it->data, "example.txt") == 0);
    it = it->next;
    assert(it->next == NULL);
    DeletePathIterator(it->head);

    printf("Testing //////dirname//subdirname///file\n");
    it = ParsePath("//////dirname//subdirname///file");
    assert(strcmp(it->data, "/") == 0);
    it = it->next;
    assert(strcmp(it->data, "dirname") == 0);
    it = it->next;
    assert(strcmp(it->data, "subdirname") == 0);
    it = it->next;
    assert(strcmp(it->data, "file") == 0);
    it = it->next;
    assert(it->next == NULL);
    DeletePathIterator(it->head);

    printf("Testing /a/b/c/\n");
    it = ParsePath("/a/b/c/");
    assert(strcmp(it->data, "/") == 0);
    it = it->next;
    assert(strcmp(it->data, "a") == 0);
    it = it->next;
    assert(strcmp(it->data, "b") == 0);
    it = it->next;
    assert(strcmp(it->data, "c") == 0);
    it = it->next;
    assert(strcmp(it->data, ".") == 0);
    it = it->next;
    assert(it->next == NULL);
    DeletePathIterator(it->head);
}
