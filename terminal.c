#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include "terminal.h"

int alt_screen(bool state) {
    return printf("\x1b[?47%c",state ? 'h':'l');
}
int go_to(unsigned int y,unsigned int x) {
    return printf("\x1b[%d;%dH",y,x);
}
int save_cur() {return printf("\x1b""7");}
int restore_cur() {return printf("\x1b""8");}
int go_home() {
    printf("\x1b[2J");
    return go_to(0,0);
}
//don't directly call this
void exit_alt() {
    alt_screen(false);
    restore_cur();
}
void enter_alt() {
    save_cur();
    alt_screen(true);
    go_home();
    if (atexit(exit_alt)!=0) {exit_alt();fprintf(stderr,"can't register with atexit()\n");exit(EXIT_FAILURE);};
}

