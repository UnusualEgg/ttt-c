#include <stdbool.h>
//#define ESC 0x1B
int alt_screen(bool state);
int go_to(unsigned int y,unsigned int x);
int save_cur();
int restore_cur();
int go_home();
//don't directly call this
void exit_alt();
void enter_alt();
