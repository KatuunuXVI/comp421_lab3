/*
 * Fills target buffer path + empty strings.
 */
void SetDirectoryName(char *target, char *path, int start, int end);

/*
 * Compare dirname. Return 0 if equal, -1 otherwise.
 */
int CompareDirname(char *dirname, char *other);
