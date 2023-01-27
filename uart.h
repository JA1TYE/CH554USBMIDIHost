#ifndef __UART_H__
#define __UART_H__

extern unsigned char __xdata wp;
extern unsigned char __xdata rp;

void initUART0(unsigned long baud, int alt);
unsigned char UART0Available(void);
void UART0ISR(void) __interrupt 4;
unsigned char UART0Receive();
void UART0Send(unsigned char b);
void initUART1(unsigned long baud, int alt);
unsigned char UART1Receive();
void UART1Send(unsigned char b);

#endif