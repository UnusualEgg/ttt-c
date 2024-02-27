//TODO fix port not closing
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>//sleep
#include <stdlib.h>//atexit
#include <string.h>//strcmp
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <signal.h>

#include <linenoise.h>

#include "socket.h"
#include "input.h"
#include "terminal.h"

#define PROJECT_NAME "ttt"

static int global_fd=-1,global_client_fd=-1;

void handler(int sig) {
    if (global_client_fd>0) {close(global_client_fd);}
    if (global_fd>0) {
        close(global_fd);
    }
}

ssize_t try_write(int fd, void* buf, size_t count) {
    int result=write(fd,buf,count);
    if (result<0) {perror("try_read");exit(EXIT_FAILURE);}
    return result;
}
ssize_t try_read(int fd, void* buf, size_t count) {
    int result=read(fd,buf,count);
    if (result<0) {perror("try_read");exit(EXIT_FAILURE);}
    return result;
}

//packets
enum packet {
    PACK_QUIT=0,
    PACK_POS=1,
};
void send_quit(int fd){
    uint32_t packet=PACK_QUIT;
    try_write(fd,&packet,sizeof(uint32_t));
}
//

void print_board(char board[3][3]) {
    for (int y=0;y<3;y++) {
        printf("%c %c %c\n",board[y][0],board[y][1],board[y][2]);
    }
}
char plrs[2] = {'o','x'};//hehe 0=o
char opposite(char p) {
    return p==plrs[0]?plrs[1]:plrs[0];
}

char check_win(char board[3][3]) {
    for (int i=0;i<2;i++) {
        char p = plrs[i];
        //horizontal&vertical
        for (int y=0;y<3;y++) {
            if (
                    p==board[y][0]&&
                    p==board[y][1]&&
                    p==board[y][2])
            {return p;}
            if (
                    p==board[0][y]&&
                    p==board[1][y]&&
                    p==board[2][y])
            {return p;}
        }
        //diagonal
        if (
                p==board[0][0]&&
                p==board[1][1]&&
                p==board[2][2])
        {return p;}
        if (
                p==board[0][2]&&
                p==board[1][1]&&
                p==board[2][0])
        {return p;}
    }
    return '\0';
}

bool check_full(char board[3][3]) {
    for (int y=0;y<3;y++) {
        for (int x=0;x<3;x++) {
            if (board[y][x]!=plrs[0]&&board[y][x]!=plrs[1]) {return false;}
        }
    }
    return true;
}

int main(int argc, char **argv) {
    //args
    bool isserver=false;
    uint16_t port=0;
    uint16_t* ptr=NULL;
    char* addr_arg=NULL;
    for (int i=1;i<argc;i++) {
        if (argv[i][0]=='-') {
            for (int j=1;argv[i][j]!='\0';j++){
                char c = argv[i][j];
                if (c=='s') {
                    isserver=true;
                } else if (c=='a') {
                    addr_arg=argv[++i];
                    if (!*addr_arg) {fprintf(stderr,"Expected adddress after -a\n");exit(EXIT_FAILURE);}
                    break;
                } else if (c=='c') {
                    if (!argv[++i][0]) {fprintf(stderr,"Expected two characters after -c\n");exit(EXIT_FAILURE);}
                    plrs[0]=argv[i][0];
                    plrs[1]=argv[i][1];
                    break;
                } else if (c=='p') {
                    if (!argv[++i][0]) {fprintf(stderr,"Expected port after -p\n");exit(EXIT_FAILURE);}
                    int result=sscanf(argv[i],"%hu",&port);
                    if (result==0) {fprintf(stderr,"invalid arg %s.\n",argv[i]);return EXIT_FAILURE;}
                    if (result==EOF) {fprintf(stderr,"invalid arg %s. Error:%s\n",argv[i],strerror(errno));return EXIT_FAILURE;}
                    ptr=&port;
                    break;
                } else {
                    fprintf(stderr,"Unknown option: \"%s\"\n",argv[i]);exit(EXIT_FAILURE);
                }
            }
        } else {
            fprintf(stderr,"Unknown argument: \"%s\"\n",argv[i]);exit(EXIT_FAILURE);
        }
    }
    //file descriptors
    int fd=-1,client_fd=-1;
    create_sock(isserver,&fd,&client_fd,addr_arg,ptr);
    int other=isserver?client_fd:fd;
    global_fd=fd;
    global_client_fd=client_fd;
    printf("created file descriptors\n");
    //signal
    struct sigaction a;
    a.sa_handler=handler;
    a.sa_flags=0;
    //a.sa_mask=(sigset_t)0;
    sigaction(SIGINT,&a,NULL);
    printf("registered handler\n");
    //setup screen
    enter_alt();
    //printf("omg haii\n");

    //ttt vars
    char board[3][3]={0};
init:
    memset(board,'.',9);

    //call rand to pick first player
    char curr,mine;
    if (isserver) {
        uint8_t currn = rand()>(RAND_MAX/2);
        curr=plrs[currn];
        //mine should be random
        uint8_t r = (rand()>(RAND_MAX/2));
        mine=plrs[r];
        uint8_t packet[2] = {currn,-r+1};
        //tell client
        printf("writing turn mine:%hu, curr:%hu\n",packet[1],packet[0]);
        fflush(stdout);
        try_write(other,packet,2);
        sleep(2);
    } else {
        uint8_t packet[2]={0,0};
        printf("reading turn\n");
        try_read(other,packet,2);
        curr=plrs[packet[0]];
        mine=plrs[packet[1]];
        printf("curr:%hu(), mine:%hu()\n",packet[0],packet[1]);
        fflush(stdout);
        sleep(2);
    }
    printf("now playing\n");
    

    //now main loop
    bool playing=true;
    while (playing) {
        go_home();
        print_board(board);
        bool check_rematch=false;
        char p=check_win(board);
        if (p) {
            printf("\n%c won.\n",p);
            check_rematch=true;
        }
        if (check_full(board)){
            printf("It's a tie.\n");
            check_rematch=true;
        }
        if (check_rematch) {
            //rematch?
            char *buf;
            char c;
            bool rematch;
            //printf();
            //fflush(stdout);
            buf=linenoise("rematch[y/n]: ");
            if (c==EOF) {
                rematch=false;
                try_write(other,&rematch,sizeof(bool));
                break;
            }
            c=buf[0];
            free(buf);
            rematch=c=='y';
            if (isserver) {
                //write then read
                try_write(other,&rematch,sizeof(bool));
                
                bool answer=false;
                try_read(other,&answer,sizeof(bool));
                rematch&=answer;
            } else {
                //read then write
                bool answer=false;
                try_read(other,&answer,sizeof(bool));
                rematch&=answer;
                
                try_write(other,&rematch,sizeof(bool));
            }

            if (!rematch) {
                //eh we already know
                //send_quit(other);
                break;
            }
            goto init;
        }
        //PLAY
        if (curr==mine) {
            printf("\nMy turn(%c)\n",mine);
            char* buf=NULL;
            
            int result=0;
            size_t len=0;
            uint8_t x=0,y=0;
            bool invalid;
            do {
                invalid=false;
                buf=linenoise("[tl/t/tr/l/c/r/bl/b/r]: ");
                if (!buf) {handler(SIGINT);exit(EXIT_FAILURE);}
                len=strlen(buf);
                result=1;
                //check if move is vald
#define invalid(msg) linenoiseClearScreen();print_board(board);printf("\nMy turn(%c)\n",mine);fprintf(stderr,msg);result=0;free(buf);continue
                if (len<1) {invalid("Error: empty input");}
                //DEBUG: printf("len:%zu (%s)\n",len,buf);
                switch (buf[0]) {
                    case 't':
                        y=0;x=1;break;
                    case 'b':
                        y=2;x=1;break;
                    case 'l':
                        x=0;y=1;break;
                    case 'c':
                        x=1;y=1;break;
                    case 'r':
                        x=2;y=1;break;
                    default:
                        invalid=true;
                }
                if (invalid) {invalid("Error: character 1 invalid\n");}
                switch (buf[1]) {
                    case 'l':
                        x=0;break;
                    case '\0':
                        break;
                    case 'r':
                        x=2;break;
                    default:
                        invalid=true;
                }
                if (invalid) {invalid("Error: character 2 invalid\n");}
                if (board[y][x]==plrs[0]||board[y][x]==plrs[1]) {result=0;fprintf(stderr,"Piece already there\n");}
                free(buf);
                buf=NULL;
            } while (!result);
            //place and send move
            board[y][x]=curr;
            uint8_t packet[3] = {PACK_POS,(uint8_t)y,(uint8_t)x};
            try_write(other,packet,sizeof(packet));
        } else {//not my turn
            printf("\nOther's turn(%c)\n",curr);
            //read turn
            uint8_t packet[3] = {0};
            try_read(other,packet,sizeof(packet));
            if (packet[0]==PACK_QUIT) {
                printf("Other quit.\n");
                break;
            }
            if (packet[0]!=PACK_POS) {
                printf("Ivalid packet.\n");
                break;
            }
            uint8_t y=packet[1],x=packet[2];
            board[y][x]=curr;
        }
        //switch turns
        curr= curr==plrs[0]?plrs[1]:plrs[0];
    }
    if (isserver) {close(client_fd);}
    close(fd);
    
    
    return 0;
}
