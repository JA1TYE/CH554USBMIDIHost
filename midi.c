#include "midi.h"
#include "uart.h"
#include "USBHost.h"
#include "CH554.h"
#include "util.h"
#include <stdio.h>

//Global variable definition
unsigned char __xdata uart_msg_buf[3] = {0};
unsigned char __xdata uart_sysex_buf[3] = {0};
unsigned char __xdata uart_parser_state = 0;
unsigned char uart_sysex_flag = 0;//special flag to handle sysex from UART to USB
//Current number of midi event in the USB packet.
//Max packet size is 64 and 1 evt = 4bytes,
//So Maximum number of events per packet is 16.  
unsigned char midi_evt_count = 0;
unsigned char status_byte = 0;

//Initialize UART-MIDI I/F
void initUARTMIDI(unsigned char mode){
    if(mode == 0){//Normal mode
        initUART0(31250,0);
    }
    else{//Fast mode
        initUART0(115200,0);
    }
    uart_parser_state = MIDI_INIT_STATE; 
}

unsigned char numArguments(unsigned char dat){
    switch(dat){
        case MSG_TUNE_REQ:
        case MSG_TIMING_CLOCK:
        case MSG_START:
        case MSG_CONTINUE:
        case MSG_STOP:
        case MSG_ACTIVE_SENSING:
        case MSG_SYSTEM_RESET:
            return 0;
            break;
        case MSG_QUARTER_FRAME: 
        case MSG_SONG_SELECT:
            return 1;
            break;
        case MSG_SONG_PTR:
            return 2;
            break;
    }

    switch(dat & MSG_TYPE_MASK){
        case MSG_PC:
        case MSG_CH_PRESSURE:
            return 1;
            break;
        case MSG_NOTE_ON:
        case MSG_NOTE_OFF:
        case MSG_POLY_PRESSURE:
        case MSG_CC:
        case MSG_PITCH_BEND:
            return 2;
            break;
        default:
            return 0;
    }
}

//Parse MIDI Bytes from UART
void parseUARTMIDIByte(unsigned char dat){ 
    //Countermeasure for unintended data byte
    if((dat & MSG_STATUS_MASK) == MSG_STATUS_MASK)uart_parser_state = MIDI_INIT_STATE;
    //Normal parse process
    if(uart_parser_state == MIDI_INIT_STATE){
        if((dat & MSG_STATUS_MASK) == MSG_STATUS_MASK){//If it is a status byte
            if((dat & MSG_SYSTEM_REALTIME_MASK) == MSG_SYSTEM_REALTIME_MASK){//And if it's a realtime msg
                if(dat == MSG_TIMING_CLOCK){
                    prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,&dat,1);
                }
                else if(dat == MSG_START){
                    prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,&dat,1);
                }
                else if(dat == MSG_CONTINUE){
                    prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,&dat,1);
                }
                else if(dat == MSG_STOP){
                    prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,&dat,1);
                }
                else if(dat == MSG_ACTIVE_SENSING){
                    prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,&dat,1);
                }
                else if(dat == MSG_SYSTEM_RESET){
                    prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,&dat,1);
                }
            }
            else{
                status_byte = dat;//set status byte
                uart_msg_buf[0] = status_byte;
                if((status_byte & MSG_TYPE_MASK) != 0xf0)status_byte &= MSG_TYPE_MASK;
                
                if(status_byte == MSG_TUNE_REQ){
                    prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,uart_msg_buf,1);
                }
                else if(status_byte == MSG_SYSEX_START){
                    //If MSbit is 1, you should send Start of SysEx before sending SysEx data
                    uart_sysex_flag = 0x80;
                }
                else if(status_byte == MSG_SYSEX_END){
                    if(uart_sysex_flag == 0x80){//Special case:F0 F7
                        uart_msg_buf[0] = MSG_SYSEX_START;
                        uart_msg_buf[1] = MSG_SYSEX_END;
                        prepareUSBMIDIPacket(CIN_2B_SYSEX,uart_msg_buf,2);
                    }
                    else if(uart_sysex_flag == 0x81){//Special case:F0 xx F7
                        uart_msg_buf[0] = MSG_SYSEX_START;
                        uart_msg_buf[1] = uart_sysex_buf[0];
                        uart_msg_buf[2] = MSG_SYSEX_END;
                        prepareUSBMIDIPacket(CIN_3B_SYSEX,uart_msg_buf,3);
                    }
                    else if(uart_sysex_flag == 0x00){
                        prepareUSBMIDIPacket(CIN_1B_SYS_1B_SYSEX,uart_msg_buf,1);
                    }
                    else if(uart_sysex_flag == 0x01){
                        uart_sysex_buf[1] = MSG_SYSEX_END;
                        prepareUSBMIDIPacket(CIN_2B_SYSEX,uart_sysex_buf,2);
                    }
                    else if(uart_sysex_flag == 0x02){
                        uart_sysex_buf[2] = MSG_SYSEX_END;
                        prepareUSBMIDIPacket(CIN_3B_SYSEX,uart_sysex_buf,3);
                    }
                }
            }
        }
        else{//Then it is 1st arg. of msg.
            uart_msg_buf[1] = dat;
            if(status_byte == MSG_PC){
                prepareUSBMIDIPacket(CIN_PC,uart_msg_buf,2);
            }
            else if(status_byte == MSG_CH_PRESSURE){
                prepareUSBMIDIPacket(CIN_CH_PRESS,uart_msg_buf,2);
            }
            else if(status_byte == MSG_QUARTER_FRAME){
                prepareUSBMIDIPacket(CIN_2B_SYS,uart_msg_buf,2);
            }
            else if(status_byte == MSG_SONG_SELECT){
                prepareUSBMIDIPacket(CIN_2B_SYS,uart_msg_buf,2);
            }
            else if(status_byte == MSG_SYSEX_START){
                //Data of SysEx
                if(uart_sysex_flag == 0x80){
                    uart_sysex_buf[0] = uart_msg_buf[1];
                    uart_sysex_flag++;
                }
                else if(uart_sysex_flag == 0x81){//F0 xx xx
                    //This is little bit tricky
                    uart_msg_buf[2] = uart_msg_buf[1];
                    uart_msg_buf[1] = uart_sysex_buf[0];
                    prepareUSBMIDIPacket(CIN_SYSEX_START_CONT,uart_msg_buf,3);
                    uart_sysex_flag = 0;                    
                }
                else{//Data of SysEx (continue)
                    uart_sysex_buf[uart_sysex_flag] = uart_msg_buf[1];
                    if(uart_sysex_flag == 0x02){
                        prepareUSBMIDIPacket(CIN_SYSEX_START_CONT,uart_sysex_buf,3);
                        uart_sysex_flag = 0;
                    }
                    else{
                        uart_sysex_flag++;
                    }
                }
            }
            else if(numArguments(status_byte) == 2){//Msg that have 2 args
                uart_parser_state = MIDI_WAIT_2ND_ARG;
            }
        }
    }
    else if(uart_parser_state == MIDI_WAIT_2ND_ARG){
        uart_msg_buf[2] = dat;
        if(status_byte == MSG_NOTE_OFF){
            prepareUSBMIDIPacket(CIN_NOTE_OFF,uart_msg_buf,3);
        }
        else if(status_byte == MSG_NOTE_ON){
            prepareUSBMIDIPacket(CIN_NOTE_ON,uart_msg_buf,3);
        }
        else if(status_byte == MSG_POLY_PRESSURE){
            prepareUSBMIDIPacket(CIN_POLY_PRESS,uart_msg_buf,3);
        }
        else if(status_byte == MSG_CC){
            prepareUSBMIDIPacket(CIN_CC,uart_msg_buf,3);
        }
        else if(status_byte == MSG_PITCH_BEND){
            prepareUSBMIDIPacket(CIN_PITCH_BEND,uart_msg_buf,3);
        }
        else if(status_byte == MSG_SONG_PTR){
            prepareUSBMIDIPacket(CIN_3B_SYS,uart_msg_buf,3);
        }
        uart_parser_state = MIDI_INIT_STATE;
    }    
}

//Parse MIDI Event Packet from USB
void parseUSBMIDIPacket(unsigned char *src,unsigned char size){
    P1 ^= (1 << 1);
    for(;size > 0;size -= 4){
        if((src[0] & CABLE_NUM_MASK) == 0){
            switch(src[0] & CODE_INDEX_MASK){
                //1-byte messages
                case CIN_1B_SYS_1B_SYSEX:
                case CIN_1B:
                    sendUARTMIDI(&src[1],1);
                    break;
                //2-bytes messages
                case CIN_2B_SYS:
                case CIN_2B_SYSEX:
                case CIN_PC:
                case CIN_CH_PRESS:
                    sendUARTMIDI(&src[1],2);
                    break;
                case CIN_3B_SYS:
                case CIN_SYSEX_START_CONT:
                case CIN_3B_SYSEX:
                case CIN_NOTE_OFF:
                case CIN_NOTE_ON:
                case CIN_POLY_PRESS:
                case CIN_CC:
                case CIN_PITCH_BEND:
                    sendUARTMIDI(&src[1],3);
                    break;
                default:
                    //Do nothing
            }
        }
        //Increment ptr
        src += 4;
    }
}

//Fill the packet by MIDI event
void prepareUSBMIDIPacket(unsigned char header,unsigned char* src, unsigned char n){
    unsigned char i;
    unsigned char idx;

    idx = midi_evt_count * 4;
    //evt[0] is Cable number /Code Index
    TxBuffer[idx] = header;
    //Fill TxBuffer by data
    for(i = 1;i <= n;i++){
        TxBuffer[idx + i] = src[i - 1];
    }
    //Set Trailing Zeros
    for(i = n + 1;i < 4;i++){
        TxBuffer[idx + i] = 0x00;
    }
    //Increment midi_evt_count
    midi_evt_count++;
    return;
}

//Send MIDI data to UART
void sendUARTMIDI(unsigned char* src, unsigned char n){
    while(n--)UART0Send(*(src++));
}