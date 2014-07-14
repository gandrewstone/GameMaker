/*
 * NT specific functions for ZIP.
 */

int GetFileMode(char *name);
long GetTheFileTime(char *name);

int IsFileNameValid(char *name);
int IsFileSystemFAT(char *dir);
void ChangeNameForFAT(char *name);

char *StringLower(char *);

char *GetLongPathEA(char *name);

