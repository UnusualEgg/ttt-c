#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

ssize_t inputs(const char* prompt,int fd, void* buf, size_t count) {
    printf("%s",prompt);
    fflush(stdout);
    ssize_t result = read(fd,buf,count);
    if (result==-1||result==0) {perror("Error on input");exit(EXIT_FAILURE);}
    return result;
}
int inputf(const char* prompt, const char* format, ...) {
    printf("%s",prompt);
    fflush(stdout);
    va_list args;
    va_start(args,format);
    int result=vscanf(format,args);
    va_end(args);
    if (result==EOF&&feof(stdin)) {fprintf(stderr,"End of file reached\n");exit(EXIT_FAILURE);}
    if (result==EOF) {perror("Error on input");exit(EXIT_FAILURE);}
    return result;
}


