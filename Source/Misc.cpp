#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <direct.h>
#include "SFFExtract.h"
#include "sffFormat.h"

#define MAKEDIR_MAXSIZE 2000

char *GetExtension(char *filePath, int maxLength)
{
	char *ext = 0;
	int pos = 0;
	while(1)
	{
		if(filePath[pos] == 0 || pos >= maxLength - 1)
			break;
		if(filePath[pos] == '.' && filePath[pos + 1] != 0)
			ext = &filePath[pos + 1];
		pos++;
	}
	return ext;
}

bool LoadFile(char *filename, unsigned char **data, unsigned int *size)
{
	FILE *file = 0;
		
	if(!file)
		fopen_s(&file, filename, "rb");
	if(file) //File load was successful
	{
		fpos_t fileSize = 0;
		_fseeki64(file, 0, SEEK_END);
		fgetpos(file, &fileSize);
		_fseeki64(file, 0, SEEK_SET);
		*data = new unsigned char [ (unsigned int) fileSize];
		fread(*data, (size_t) fileSize, 1, file);
		*size = (unsigned int) fileSize;
		fclose(file);
		return 1;
	}
}

//Make directory. This version handles a path with a file, so it'll not add a directory with file name. (TODO: This should be obsolete! Replace this with below function and confirm code still works, then remove this function)
void MakeDirectory_PathEndsWithFile(char *fullpath, int pos)
{
	char path[MAKEDIR_MAXSIZE];
	memset(path, 0, MAKEDIR_MAXSIZE);

	if(pos != 0)
		memcpy(path, fullpath, pos);

	while(1)
	{
		if(fullpath[pos] == 0)
			break;
		path[pos] = fullpath[pos];
		pos++;
		if(fullpath[pos] == '\\' || fullpath[pos] == '/')
			_mkdir(path);
	}
}