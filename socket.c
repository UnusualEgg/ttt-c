#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>

#include "input.h"
#include "socket.h"

void print_ifa() {
    //from https://stackoverflow.com/questions/2283494/get-ip-address-of-an-interface-on-linux
    struct ifaddrs *ifaddr, *ifa;
    int code;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }


    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) {continue;}  

        code=getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in),host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

        if(!(ifa->ifa_flags&IFF_LOOPBACK)&&(ifa->ifa_addr->sa_family==AF_INET))
        {
            if (code != 0)
            {
                printf("getnameinfo() failed: %s\n", gai_strerror(code));
                exit(EXIT_FAILURE);
            }
            printf("\tInterface : <%s>\n",ifa->ifa_name );
            printf("\t  Address : <%s>\n", host); 
        }
    }

    freeifaddrs(ifaddr);
}

void print_addr(struct in_addr *addr) {
    char buf[INET_ADDRSTRLEN]={0};
    if(!inet_ntop(AF_INET,addr,buf,(socklen_t)sizeof(buf))){perror("Failed to convert addr to str");exit(EXIT_FAILURE);};
    printf("Address: %s\n",buf);
}

void create_sock(bool isserver,int* fd, int* client_fd,char* addr_arg, uint16_t* port) {
    //parse addr_arg
    struct sockaddr_in addr={0};
    addr.sin_family=AF_INET;
    addr.sin_port=0;
    if (addr_arg) {
        if (!inet_pton(AF_INET,addr_arg,&(addr.sin_addr))){fprintf(stderr,"invalid address argument\n");exit(EXIT_FAILURE);}
    }

    //make socket
    *fd = socket(AF_INET,SOCK_STREAM,0);
    if (*fd==-1) {perror("Couldn't create socket");exit(EXIT_FAILURE);}
    //get address first
    //get addr from stdin
    if (!addr_arg) {
        if (isserver) {
            //print our options
            print_ifa();
        }
        int result=0;
        char buf[INET_ADDRSTRLEN]={0};
        do {
            inputs("Address: ",STDIN_FILENO,buf,sizeof(buf));
            char* nl=strstr(buf,"\n");
            if (nl) {*nl='\0';}
            printf("got \"%s\"\n",buf);
            result=inet_pton(AF_INET,buf,&(addr.sin_addr));
            if (!result) {fprintf(stderr,"Conversion error: %s\n",strerror(errno));}
        } while (!result);
    }
    //get port
    if (port) {
        addr.sin_port=*port;
    } else {
        //get port from stdin
        int result=0;
        do {
            result=inputf("Port:","%hu",&(addr.sin_port));
            //if (result==0) {fprintf(stderr,"invalid arg %s.\n",argv[i]);return EXIT_FAILURE;}
            if (result==EOF) {fprintf(stderr,"invalid arg %s. Error:%s\n",addr_arg,strerror(errno));exit(EXIT_FAILURE);}
        } while (!result);
    }
    //double check
    printf("we got:\n\t");
    print_addr(&(addr.sin_addr));
    printf("\tPort: %hu\n",addr.sin_port);

    if (isserver) {
        //now bind
        if (bind(*fd,(struct sockaddr*)&addr,sizeof(addr))==-1) {perror("Couldn't bind");exit(EXIT_FAILURE);}
        //listen
        if (listen(*fd,1)==-1){perror("listen");exit(EXIT_FAILURE);}
        //accpet
        int result=accept(*fd,NULL,NULL);
        if (result==-1) {perror("accept");exit(EXIT_FAILURE);}
        *client_fd=result;
    } else {
        //connect
        if (connect(*fd,(struct sockaddr*)&addr,(socklen_t)sizeof(addr))) {perror("connect");exit(EXIT_FAILURE);}
    }
    //int sd=isserver?*client_fd:*fd;
    //now we conencted
    //test recv and send
    //char msg[] = "Hewro";
    //ssize_t written=write(sd,msg,sizeof(msg));
    //if (written<0) {perror("write");exit(EXIT_FAILURE);}
    //char buf[10]={0};
    //ssize_t len = read(sd,buf,sizeof(buf));
    //if (len==-1){perror("read");exit(EXIT_FAILURE);}
    //printf("Got from read \"%s\"\n",buf);
}
