#include <stdio.h>

extern void* info(char* str);
extern void* error(char* str);

extern int OpenLogFile(const char* filepath);
extern int CloseLogFile();
extern int WriteLog(char* log);
