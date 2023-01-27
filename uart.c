#include "CH554.h"
#include "uart.h"

//16-depth ring buf
#define UART0_RX_BUF_DEPTH 16
unsigned char __xdata uart0_buf[UART0_RX_BUF_DEPTH];
unsigned char __xdata rp;
unsigned char __xdata wp;


/**
 * Initialize UART0 port with given baud rate
 * pins: tx = P3.1 rx = P3.0
 * alt != 0 pins: tx = P1.3 rx = P1.2
 */

void initUART0(unsigned long baud, int alt)
{
	unsigned long x;
	if(alt){
		P1_MOD_OC &= (~bTXD_);
		P1_DIR_PU |= bTXD_ | bRXD_;
		PIN_FUNC |= bUART0_PIN_X;
	}
	else{
		P3_MOD_OC &= (~bTXD);
		P3_DIR_PU |= bTXD | bRXD;
		PIN_FUNC &= (~bUART0_PIN_X);
	}

 	SM0 = 0;
	SM1 = 1;
	SM2 = 0;
	REN = 1;
    RCLK = 0;
    TCLK = 0;
    PCON |= SMOD;
	//Register TH1 determines the baud rate.
	//TH1 value should be 256-Fsys/16/(baud rate) when TCLK = 0,RCLK = 0,
	//bTMR_CLK = 1, bT1_CLK = 1 in T2MOD
	//this fomula is equivalent to round(FREQ_SYS/16/baud)
    x = (((unsigned long)FREQ_SYS / 8) / baud + 1) / 2;

	//Timer 1 is used as a 8-bit auto-reload counter
    TMOD = TMOD & ~ bT1_GATE & ~ bT1_CT & ~ MASK_T1_MOD | bT1_M1;
    T2MOD = T2MOD | bTMR_CLK | bT1_CLK;
    TH1 = (256 - x) & 255;
    TR1 = 1;//Start Timer 1
	TI = 0;

    //Enable Interrupt
    EA = 1;
    ES = 1;

    rp = 0;
    wp = 0;

}

unsigned char UART0Available(void){
    return (wp >= rp)?(wp - rp):(wp + UART0_RX_BUF_DEPTH - rp); 
}

unsigned char UART0Receive(void){
    unsigned char ret;
    if(UART0Available() == 0){
        return 0;
    }
    else{
        ret = uart0_buf[rp];
        rp = (rp == (UART0_RX_BUF_DEPTH - 1))?0:rp + 1;
    }
    return ret;
}

//Interrupt 4 is UART0 interrupt
void UART0ISR(void) __interrupt 4{
    if(RI == 1){//check interrupt cause
		uart0_buf[wp] = SBUF;
		wp = (wp == (UART0_RX_BUF_DEPTH - 1))?0:wp + 1;
		RI = 0;
	}
	//TI bit should be cleared by UART0Send
}

void UART0Send(unsigned char b)
{
	SBUF = b;
	while(TI == 0);
	TI = 0;
}

/**
 * Initialize UART1 port with given baud rate
 * pins: tx = P1.7 rx = P1.6
 * alt != 0 pins: tx = P3.2 rx = P3.4
 */

void initUART1(unsigned long baud, int alt)
{
	unsigned long x;
	if(alt){
		P1_MOD_OC &= (~bTXD1_);
		P1_DIR_PU |= bTXD1_ | bRXD1_;
		PIN_FUNC |= bUART1_PIN_X;
	}
	else{
		P3_MOD_OC &= (~bTXD1);
		P3_DIR_PU |= bTXD1 | bRXD1;
		PIN_FUNC &= (~bUART1_PIN_X);
	}

 	U1SM0 = 0;
	U1REN = 1;
	U1SMOD = 1;
	//Register SBAUD1 determines the baud rate.
	//SBAUD1 value should be 256-Fsys/16/(baud rate)
	//this fomula is equivalent to round(FREQ_SYS/16/baud)
    x = (((unsigned long)FREQ_SYS / 8) / baud + 1) / 2;

    SBAUD1 = (256 - x) & 255;
	U1TI = 0;
}

unsigned char UART1Receive()
{
    while(U1RI == 0);
    U1RI = 0;
    return SBUF1;
}

void UART1Send(unsigned char b)
{
	SBUF1 = b;
	while(U1TI == 0);
	U1TI = 0;
}
