#include <stdarg.h>
#include <unistd.h>

ssize_t inputs(const char* prompt,int fd, void* buf, size_t count);
int inputf(const char* prompt, const char* format, ...);


