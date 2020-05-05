#include <comp421/filesystem.h>

/*
 * Fills target buffer path + empty strings.
 */
 void SetDirectoryName(char *target, char *path, int start, int end) {
     int i;
     for (i = 0; i < DIRNAMELEN; i++) {
         if (i < end - start) target[i] = path[start + i];
         else target[i] = '\0';
     }
 }

/*
 * Compare dirname. Return 0 if equal, -1 otherwise.
 */
 int CompareDirname(char *dirname, char *other) {
     int i;
     for (i = 0; i < DIRNAMELEN; i++) {
         /* If characters do not match, it is not equal */
         if (dirname[i] != other[i]) return -1;

         /* If dirname is null string, break */
         if (dirname[i] == '\0') break;
     }

     return 0;
 }
