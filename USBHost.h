#ifndef __USBHOST_H__
#define __USBHOST_H__

#include "CH554.h"

#define MAX_INTERFACE_COUNT     4
#define MAX_ENDPOINT_COUNT      4
#define ENDPOINT_OUT            0
#define ENDPOINT_IN             1

#define ERR_SUCCESS         0x00
#define ERR_USB_CONNECT     0x15
#define ERR_USB_DISCON      0x16
#define ERR_USB_BUF_OVER    0x17
#define ERR_USB_DISK_ERR    0x1F
#define ERR_USB_TRANSFER    0x20 
#define ERR_USB_UNSUPPORT   0xFB
#define ERR_USB_UNKNOWN     0xFE

#define ROOT_DEVICE_DISCONNECT  0
#define ROOT_DEVICE_CONNECTED   1
#define ROOT_DEVICE_FAILED      2
#define ROOT_DEVICE_SUCCESS     3

typedef struct _EndPoint
{
	unsigned char EndpointAddr;
	unsigned short MaxPacketSize;
	
	unsigned char EndpointDir : 1;
	unsigned char TOG : 1;
} EndPoint, *PEndPoint;

typedef struct _Interface
{
	unsigned char       InterfaceClass;
	unsigned char       InterfaceProtocol;
	
	unsigned char       EndpointCount;
	EndPoint            endpoint[MAX_ENDPOINT_COUNT];
	
} Interface, *PInterface;

typedef struct _UsbDevice
{
	unsigned char   DeviceClass;
	unsigned char   MaxPacketSize0;
	
	unsigned short  VendorID;
	unsigned short  ProductID;
	unsigned short  bcdDevice;

	unsigned char   DeviceAddress;
	unsigned char   DeviceSpeed;
	unsigned char   InterfaceCount;
	Interface       interface[MAX_INTERFACE_COUNT];

	unsigned char   HubPortNum;
} UsbDevice, *PUsbDevice;

void resetRootHub();
void initUSB_Host();
unsigned char checkRootHubConnections();

void resetHubDevices();
void pollMIDIdevice();

extern __at(0x0000) unsigned char __xdata RxBuffer[MAX_PACKET_SIZE];
extern __at(0x0040) unsigned char __xdata TxBuffer[MAX_PACKET_SIZE];

#endif