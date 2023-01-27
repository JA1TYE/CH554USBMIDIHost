#ifndef __UTIL_H__
#define __UTIL_H__

void initClock();
void delayUs(unsigned short n);
void delay(unsigned short n);

int putchar(int c);
int getchar();
char* u8ToHEX(unsigned char value,char *str);
#define PIN_MODE_INPUT 0
#define PIN_MODE_PP_OUTPUT 1
#define PIN_MODE_OD_INOUT 2
#define PIN_MODE_BIDIR_INOUT 3

void pinMode(unsigned char port, unsigned char pin, unsigned char mode);

#endif