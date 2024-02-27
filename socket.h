#include <stdbool.h>
#include <sys/types.h>

void print_ifa();
void print_addr(struct in_addr *addr);
void create_sock(bool isserver,int* fd, int* client_fd,char* addr_arg, uint16_t* port);

