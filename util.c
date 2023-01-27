#include "CH554.h"
#include "util.h"
#include "uart.h"

#ifndef FREQ_SYS
#define	FREQ_SYS	24000000
#endif 

void initClock()
{
    SAFE_MOD = 0x55;
    SAFE_MOD = 0xAA;

	CLOCK_CFG &= ~MASK_SYS_CK_SEL;
	CLOCK_CFG |= 6;//System Clock = Fpll/4, 24MHz 															  

    SAFE_MOD = 0xFF;

	delay(7);
}

/**
* #define PIN_MODE_INPUT 0
* #define PIN_MODE_PP_OUTPUT 1
* #define PIN_MODE_OD_INOUT 2
* #define PIN_MODE_BIDIR_INOUT 3
 */
void pinMode(unsigned char port, unsigned char pin, unsigned char mode)
{
	volatile unsigned char *dir[] = {0, &P1_MOD_OC, 0, &P3_MOD_OC};
	volatile unsigned char *pu[] = {0, &P1_DIR_PU, 0, &P3_DIR_PU};
	if(port > 3)return;
	if(dir[port] == 0)return;
	switch (mode)
	{
	case PIN_MODE_INPUT: //Input only, no pull up
		*dir[port] &= ~(1 << pin);
		*pu[port] &= ~(1 << pin);
		break;
	case PIN_MODE_PP_OUTPUT: //Push-Pull output
		*dir[port] &= ~(1 << pin);
		*pu[port] |= 1 << pin;
		break;
	case PIN_MODE_OD_INOUT: //Open drain in/out no pull-up
		*dir[port] |= 1 << pin;
		*pu[port] &= ~(1 << pin);
		break;
	case PIN_MODE_BIDIR_INOUT: //Quasi-bidir mode, OD in/out with pull-up
		*dir[port] |= 1 << pin;
		*pu[port] |= 1 << pin;
		break;
	default:
		break;
	}
}

/**
 * stdio printf directed to UART0 using putchar and getchar
 */

int putchar(int c)
{
	UART1Send((unsigned char)c & 0xff);
	return c;
}

int getchar() 
{
	return (int)UART1Receive();
}

char* u8ToHEX(unsigned char value,char *str){
	const char tbl[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
	str[0] = tbl[value >> 4];
	str[1] = tbl[value & 0x0f];
	str[2] = '\0';
	return str;
}

/*******************************************************************************
* Function Name  : delayUs(UNIT16 n)
* Description    : us
* Input          : UNIT16 n
* Output         : None
* Return         : None
*******************************************************************************/ 
void	delayUs(unsigned short n)
{
	while (n) 
	{  // total = 12~13 Fsys cycles, 1uS @Fsys=12MHz
		++ SAFE_MOD;  // 2 Fsys cycles, for higher Fsys, add operation here
#ifdef	FREQ_SYS
#if		FREQ_SYS >= 14000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 16000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 18000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 20000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 22000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 24000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 26000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 28000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 30000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 32000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 34000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 36000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 38000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 40000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 42000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 44000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 46000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 48000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 50000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 52000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 54000000
		++ SAFE_MOD;
#endif
#if		FREQ_SYS >= 56000000
		++ SAFE_MOD;
#endif
#endif
		--n;
	}
}

/*******************************************************************************
* Function Name  : delay(UNIT16 n)
* Description    : ms
* Input          : UNIT16 n
* Output         : None
* Return         : None
*******************************************************************************/
void delay(unsigned short n)
{
	while (n) 
	{
		delayUs(1000);
		--n;
	}
}