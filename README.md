# CH554 USB-MIDI Host Example

This example implements a converter between USB-MIDI Host and (conventional) MIDI 1.0 on [WCH's 8-bit MCU CH554](http://wch-ic.com/products/CH554.html).

This source code is derived from [CH559sdccUSBHost](https://github.com/atc1441/CH559sdccUSBHost).

PCB design data for this example is availabe on [CH554USBMIDIBoard](https://github.com/JA1TYE/CH554USBMIDIBoard).

## How to use
CH554's UART0(TX:P3.1/RX:P3.0) are used as MIDI 1.0 I/F. 
(You should add TX/RX circuit to comply MIDI 1.0 standards.) 

CH554's UART1(TX:P1.7/RX:P1.6) are used for debugging.

MIDI events from USB-MIDI device on the cable index 0 (Most of USB-MIDI device uses cable index 0) are converted into MIDI 1.0 messages and send to UART0 and vice versa.

Currently, this example doesn't support USB-MIDI device that have a USB Hub (ex. KORG microKEY 1st model etc.)

## Tested USB-MIDI Device
- microKEY2 (KORG)
- nanoKEY2/nanoKONTROL2(KORG)
- NSX-39(Gakken)
