#ifndef __MIDI_H__
#define __MIDI_H__

#define CIN_MISC             0x00
#define CIN_CABLE_EVT        0x01
#define CIN_2B_SYS           0x02
#define CIN_3B_SYS           0x03
#define CIN_SYSEX_START_CONT 0x04
#define CIN_1B_SYS_1B_SYSEX  0x05
#define CIN_2B_SYSEX         0x06
#define CIN_3B_SYSEX         0x07
#define CIN_NOTE_OFF         0x08
#define CIN_NOTE_ON          0x09
#define CIN_POLY_PRESS       0x0a
#define CIN_CC               0x0b
#define CIN_PC               0x0c
#define CIN_CH_PRESS         0x0d
#define CIN_PITCH_BEND       0x0e
#define CIN_1B               0x0f

#define CABLE_NUM_MASK       0xf0
#define CODE_INDEX_MASK      0x0f

//MIDI messages definition
#define MSG_STATUS_MASK 0x80
#define MSG_CH_MASK 0x0f
#define MSG_TYPE_MASK 0xf0
#define MSG_NOTE_ON 0x90
#define MSG_NOTE_OFF 0x80
#define MSG_POLY_PRESSURE 0xA0
#define MSG_CC 0xB0
#define MSG_PC 0xC0
#define MSG_CH_PRESSURE 0xD0
#define MSG_PITCH_BEND 0xE0
#define MSG_SYSTEM_MASK 0xF0
#define MSG_SYSEX_START 0xF0
//MIDI System Common Message
#define MSG_QUARTER_FRAME 0xF1
#define MSG_SONG_PTR 0xF2
#define MSG_SONG_SELECT 0xF3
#define MSG_TUNE_REQ 0xF6
#define MSG_SYSEX_END 0xF7
//MIDI System Realtime Message
#define MSG_SYSTEM_REALTIME_MASK 0xF8
#define MSG_TIMING_CLOCK 0xF8
#define MSG_START 0xFA
#define MSG_CONTINUE 0xFB
#define MSG_STOP 0xFC
#define MSG_ACTIVE_SENSING 0xFE
#define MSG_SYSTEM_RESET 0xFF

//Parser status definition
#define MIDI_INIT_STATE 0
#define MIDI_WAIT_2ND_ARG 1

void initUARTMIDI(unsigned char mode);
unsigned char numArguments(unsigned char dat);
void parseUARTMIDIByte(unsigned char dat);
void parseUSBMIDIPacket(unsigned char *src,unsigned char size);
void prepareUSBMIDIPacket(unsigned char header,unsigned char* src, unsigned char n);
void sendUARTMIDI(unsigned char* src, unsigned char n);

extern unsigned char midi_evt_count;


#endif