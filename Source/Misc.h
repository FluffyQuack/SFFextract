#pragma once

#define MAX_PATH 260

char *GetExtension(char *filePath, int maxLength);
bool LoadFile(char *filename, unsigned char **data, unsigned int *size);
void MakeDirectory_PathEndsWithFile(char *fullpath, int pos = 0);