typedef unsigned char *PUINT8;
typedef unsigned char __xdata *PUINT8X;
typedef const unsigned char __code *PUINT8C;
typedef unsigned char __xdata UINT8X;
typedef unsigned char  __data             UINT8D;

#include <stdint.h>
#include <stdio.h>
#include "CH554.h"
#include "util.h"
#include "USBHost.h"
#include "uart.h"
#include "midi.h"

void main()
{
    unsigned char s;
    initClock();
    pinMode(1,1,PIN_MODE_PP_OUTPUT);
    P1 |= 2;  
    initUARTMIDI(0);
    initUART1(115200, 0);
    puts("Startup\n");
    resetHubDevices();
    initUSB_Host();
    while(1)
    {
        s = checkRootHubConnections();
        pollMIDIdevice();
    }
}