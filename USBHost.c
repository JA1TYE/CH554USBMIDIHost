#include "CH554.h"
#include "USBHost.h"
#include "util.h"
#include "uart.h"
#include "midi.h"
#include <stdio.h>
#include <string.h>

SBIT(LED, 0x90, 6);

typedef const unsigned char __code *PUINT8C;

__code unsigned char GetDeviceDescriptorRequest[] = {USB_REQ_TYP_IN, USB_GET_DESCRIPTOR, 0, USB_DESCR_TYP_DEVICE, 0, 0, sizeof(USB_DEV_DESCR), 0};
__code unsigned char GetConfigurationDescriptorRequest[] = {USB_REQ_TYP_IN, USB_GET_DESCRIPTOR, 0, USB_DESCR_TYP_CONFIG, 0, 0, sizeof(USB_DEV_DESCR), 0};
__code unsigned char GetInterfaceDescriptorRequest[] = {USB_REQ_TYP_IN | USB_REQ_RECIP_INTERF, USB_GET_DESCRIPTOR, 0, USB_DESCR_TYP_INTERF, 0, 0, sizeof(USB_ITF_DESCR), 0};
__code unsigned char SetUSBAddressRequest[] = {USB_REQ_TYP_OUT, USB_SET_ADDRESS, USB_DEVICE_ADDR, 0, 0, 0, 0, 0};
__code unsigned char GetDeviceStringRequest[] = {USB_REQ_TYP_IN, USB_GET_DESCRIPTOR, 2, 3, 9, 4, 2, 4}; // todo change language
__code unsigned char SetupSetUsbConfig[] = {USB_REQ_TYP_OUT, USB_SET_CONFIGURATION, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

__code unsigned char SetHIDIdleRequest[] = {USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF, HID_SET_IDLE, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
__code unsigned char GetHIDReport[] = {USB_REQ_TYP_IN | USB_REQ_RECIP_INTERF, USB_GET_DESCRIPTOR, 0x00, USB_DESCR_TYP_REPORT, 0 /*interface*/, 0x00, 0xff, 0x00};

__at(0x0000) unsigned char __xdata RxBuffer[MAX_PACKET_SIZE];
__at(0x0040) unsigned char __xdata TxBuffer[MAX_PACKET_SIZE];

__xdata uint8_t endpoint0Size; // todo rly global?
unsigned char SetPort = 0;	   // todo really global?

#define RECEIVE_BUFFER_LEN 256
__xdata unsigned char receiveDataBuffer[RECEIVE_BUFFER_LEN];

struct _RootHubDevice
{
	unsigned char status;
	unsigned char address;
	unsigned char speed;
} __xdata rootHubDevice;

void disableRootHubPort()
{
	rootHubDevice.status = ROOT_DEVICE_DISCONNECT;
	rootHubDevice.address = 0;
}

void initUSB_Host()
{
	IE_USB = 0;
	USB_CTRL = bUC_HOST_MODE;
	UHOST_CTRL &= ~bUH_PD_DIS;
	USB_DEV_AD = 0x00;
	UH_EP_MOD = bUH_EP_TX_EN | bUH_EP_RX_EN;
	UH_RX_DMA = 0x0000;
	UH_TX_DMA_H = 0x00;
	UH_TX_DMA_L = 0x40;
	UH_RX_CTRL = 0x00;
	UH_TX_CTRL = 0x00;
	USB_CTRL = bUC_HOST_MODE | bUC_INT_BUSY | bUC_DMA_EN;
	UH_SETUP = bUH_SOF_EN;
	USB_INT_FG = 0xFF;

	disableRootHubPort();
	USB_INT_EN = bUIE_TRANSFER | bUIE_DETECT;
}

void setHostUsbAddr(unsigned char addr)
{
	USB_DEV_AD = USB_DEV_AD & bUDA_GP_BIT | addr & 0x7F;
}

void setUsbSpeed(unsigned char fullSpeed)
{
	if (fullSpeed)
	{
		USB_CTRL &= ~bUC_LOW_SPEED;
		UH_SETUP &= ~bUH_PRE_PID_EN;
	}
	else
		USB_CTRL |= bUC_LOW_SPEED;
}

void resetRootHubPort()
{
	endpoint0Size = DEFAULT_ENDP0_SIZE; // todo what's that?
	setHostUsbAddr(0);
	UHOST_CTRL &= ~bUH_PORT_EN;
	setUsbSpeed(1);
	UHOST_CTRL = UHOST_CTRL & ~bUH_LOW_SPEED | bUH_BUS_RESET;
	delay(15);
	UHOST_CTRL = UHOST_CTRL & ~bUH_BUS_RESET;
	delayUs(250);
	UIF_DETECT = 0; // todo test if redundant
}

unsigned char enableRootHubPort()
{
	if (rootHubDevice.status < 1)
	{
		rootHubDevice.status = 1;
	}
	if (USB_MIS_ST & bUMS_DEV_ATTACH)
	{
		if ((UHOST_CTRL & bUH_PORT_EN) == 0x00)
		{
			if (USB_MIS_ST & bUMS_DM_LEVEL)
			{
				rootHubDevice.speed = 0;
				UHOST_CTRL |= bUH_LOW_SPEED;
			}
			else
				rootHubDevice.speed = 1;
		}
		UHOST_CTRL |= bUH_PORT_EN;
		return ERR_SUCCESS;
	}
	return ERR_USB_DISCON;
}

void selectHubPort()
{
	setHostUsbAddr(rootHubDevice.address); // todo ever != 0
	setUsbSpeed(rootHubDevice.speed);	   // isn't that set before?
}

unsigned char hostTransfer(unsigned char endp_pid, unsigned char tog, unsigned short timeout)
{
	unsigned short retries;
	unsigned char r;
	unsigned short i;
	UH_RX_CTRL = tog;
	UH_TX_CTRL = tog;
	retries = 0;
	do
	{
		UH_EP_PID = endp_pid;
		UIF_TRANSFER = 0;
		for (i = 200; i != 0 && UIF_TRANSFER == 0; i--)
			delayUs(1);
		UH_EP_PID = 0x00;
		if (UIF_TRANSFER == 0)
		{
			return ERR_USB_UNKNOWN;
		}
		if (UIF_TRANSFER)
		{
			if (U_TOG_OK)
			{
				return (ERR_SUCCESS);
			}
			r = USB_INT_ST & MASK_UIS_H_RES;
			if (r == USB_PID_STALL)
			{
				puts("UIF_TRAN_STALL\n");
				return (r | ERR_USB_TRANSFER);
			}
			if (r == USB_PID_NAK)
			{
				if (timeout == 0)
				{
					return (r | ERR_USB_TRANSFER);
				}
				if (timeout < 0xFFFF)
				{
					timeout--;
				}
				retries--;
			}
			else
			{
				switch (endp_pid >> 4)
				{ // todo no return.. compare to other guy
				case USB_PID_SETUP:
				case USB_PID_OUT:
					if (U_TOG_OK)
					{
						return (ERR_SUCCESS);
					}
					if (r == USB_PID_ACK)
					{
						puts("UIF_TRAN_E0\n");
						return (ERR_SUCCESS);
					}
					if (r == USB_PID_STALL || r == USB_PID_NAK)
					{
						puts("UIF_TRAN_E1\n");
						return (r | ERR_USB_TRANSFER);
					}
					if (r)
					{
						puts("UIF_TRAN_E2\n");
						return (r | ERR_USB_TRANSFER);
					}
					break;
				case USB_PID_IN:
					if (U_TOG_OK)
					{
						return (ERR_SUCCESS);
					}
					if (tog ? r == USB_PID_DATA1 : r == USB_PID_DATA0)
					{
						return (ERR_SUCCESS);
					}
					if (r == USB_PID_STALL || r == USB_PID_NAK)
					{
						puts("UIF_TRAN_E3\n");
						return (r | ERR_USB_TRANSFER);
					}
					if (r == USB_PID_DATA0 && r == USB_PID_DATA1)
					{
					}
					else if (r)
					{
						puts("UIF_TRAN_E4\n");
						return (r | ERR_USB_TRANSFER);
					}
					break;
				default:
					puts("UIF_TRAN_DEF\n");
					return (ERR_USB_UNKNOWN);
					break;
				}
			}
		}
		else
			{
				USB_INT_FG = 0xFF;
			}
			delayUs(15);
		}
		while (++retries < 200);
		return (ERR_USB_TRANSFER);
	}

	// todo request buffer
	unsigned char hostCtrlTransfer(unsigned char __xdata *DataBuf, unsigned short *RetLen, unsigned short maxLength)
	{
		unsigned char temp = maxLength;
		unsigned short RemLen;
		unsigned char s, RxLen, i;
		unsigned char __xdata *pBuf;
		unsigned short *pLen;
		PXUSB_SETUP_REQ pSetupReq = ((PXUSB_SETUP_REQ)TxBuffer);
		pBuf = DataBuf;
		pLen = RetLen;
		delayUs(200);
		if (pLen)
			*pLen = 0;
		UH_TX_LEN = sizeof(USB_SETUP_REQ);
		s = hostTransfer((unsigned char)(USB_PID_SETUP << 4), 0, 10000);
		if (s != ERR_SUCCESS)
			return (s);
		UH_RX_CTRL = UH_TX_CTRL = bUH_R_TOG | bUH_R_AUTO_TOG | bUH_T_TOG | bUH_T_AUTO_TOG;
		UH_TX_LEN = 0x01;
		RemLen = (pSetupReq->wLengthH << 8) | (pSetupReq->wLengthL);
		if (RemLen && pBuf)
		{
			if (pSetupReq->bRequestType & USB_REQ_TYP_IN)
			{
				while (RemLen)
				{
					delayUs(300);
					s = hostTransfer((unsigned char)(USB_PID_IN << 4), UH_RX_CTRL, 10000);
					if (s != ERR_SUCCESS)
						return (s);
					RxLen = USB_RX_LEN < RemLen ? USB_RX_LEN : RemLen;
					RemLen -= RxLen;
					if (pLen)
						*pLen += RxLen;
					for (i = 0; i < RxLen; i++)
						pBuf[i] = RxBuffer[i];
					pBuf += RxLen;
					if (USB_RX_LEN == 0 || (USB_RX_LEN < endpoint0Size))
						break;
				}
				UH_TX_LEN = 0x00;
			}
			else
			{
				// todo rework this TxBuffer overwritten
				while (RemLen)
				{
					delayUs(200);
					UH_TX_LEN = RemLen >= endpoint0Size ? endpoint0Size : RemLen;
					// memcpy(TxBuffer, pBuf, UH_TX_LEN);
					pBuf += UH_TX_LEN;
					if (pBuf[1] == 0x09)
					{
						SetPort = SetPort ^ 1 ? 1 : 0;
						*pBuf = SetPort;
					}
					s = hostTransfer(USB_PID_OUT << 4, UH_TX_CTRL, 10000);
					if (s != ERR_SUCCESS)
						return (s);
					RemLen -= UH_TX_LEN;
					if (pLen)
						*pLen += UH_TX_LEN;
				}
			}
		}
		delayUs(200);
		s = hostTransfer((UH_TX_LEN ? USB_PID_IN << 4 : USB_PID_OUT << 4), bUH_R_TOG | bUH_T_TOG, 10000);
		if (s != ERR_SUCCESS)
			return (s);
		if (UH_TX_LEN == 0)
			return (ERR_SUCCESS);
		if (USB_RX_LEN == 0)
			return (ERR_SUCCESS);
		return (ERR_USB_BUF_OVER);
	}

	void fillTxBuffer(PUINT8C data, unsigned char len)
	{
		unsigned char i;
		for (i = 0; i < len; i++)
			TxBuffer[i] = data[i];
	}

	unsigned char getDeviceDescriptor()
	{
		unsigned char s;
		unsigned short len;
		endpoint0Size = DEFAULT_ENDP0_SIZE;
		fillTxBuffer(GetDeviceDescriptorRequest, sizeof(GetDeviceDescriptorRequest));
		s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
		if (s != ERR_SUCCESS)
			return s;

		endpoint0Size = ((PXUSB_DEV_DESCR)receiveDataBuffer)->bMaxPacketSize0;
		if (len < ((PUSB_SETUP_REQ)GetDeviceDescriptorRequest)->wLengthL)
		{
			return ERR_USB_BUF_OVER;
		}
		return ERR_SUCCESS;
	}

	unsigned char setUsbAddress(unsigned char addr)
	{
		unsigned char s;
		PXUSB_SETUP_REQ pSetupReq = ((PXUSB_SETUP_REQ)TxBuffer);
		fillTxBuffer(SetUSBAddressRequest, sizeof(SetUSBAddressRequest));
		pSetupReq->wValueL = addr;
		s = hostCtrlTransfer(0, 0, 0);
		if (s != ERR_SUCCESS)
			return s;
		setHostUsbAddr(addr);
		delay(100);
		return ERR_SUCCESS;
	}

	unsigned char setUsbConfig(unsigned char cfg)
	{
		PXUSB_SETUP_REQ pSetupReq = ((PXUSB_SETUP_REQ)TxBuffer);
		fillTxBuffer(SetupSetUsbConfig, sizeof(SetupSetUsbConfig));
		pSetupReq->wValueL = cfg;
		return (hostCtrlTransfer(0, 0, 0));
	}

	unsigned char getDeviceString()
	{
		fillTxBuffer(GetDeviceStringRequest, sizeof(GetDeviceStringRequest));
		return hostCtrlTransfer(receiveDataBuffer, 0, RECEIVE_BUFFER_LEN);
	}

	unsigned char getConfigurationDescriptor()
	{
		unsigned char s;
		unsigned short len, total;
		fillTxBuffer(GetConfigurationDescriptorRequest, sizeof(GetConfigurationDescriptorRequest));

		s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
		if (s != ERR_SUCCESS)
			return s;
		// todo didnt send reqest completely
		if (len < ((PUSB_SETUP_REQ)GetConfigurationDescriptorRequest)->wLengthL)
			return ERR_USB_BUF_OVER;

		// todo fix 16bits
		total = ((PXUSB_CFG_DESCR)receiveDataBuffer)->wTotalLengthL + (((PXUSB_CFG_DESCR)receiveDataBuffer)->wTotalLengthH << 8);
		fillTxBuffer(GetConfigurationDescriptorRequest, sizeof(GetConfigurationDescriptorRequest));
		((PUSB_SETUP_REQ)TxBuffer)->wLengthL = (unsigned char)(total & 255);
		((PUSB_SETUP_REQ)TxBuffer)->wLengthH = (unsigned char)(total >> 8);
		s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
		if (s != ERR_SUCCESS)
			return s;
		// todo 16bit and fix received length check
		// if (len < total || len < ((PXUSB_CFG_DESCR)receiveDataBuffer)->wTotalLengthL)
		//     return( ERR_USB_BUF_OVER );
		return ERR_SUCCESS;
	}

	unsigned char getInterfaceDescriptor()
	{
		unsigned char s;
		unsigned short len;
		fillTxBuffer(GetInterfaceDescriptorRequest, sizeof(GetInterfaceDescriptorRequest));
		s = hostCtrlTransfer(receiveDataBuffer, &len, RECEIVE_BUFFER_LEN);
		return s;
	}

	struct
	{
		unsigned char connected;
		unsigned char rootHub;
		unsigned char interface;
		unsigned char inEndPoint;
		unsigned char outEndPoint;
		unsigned long type;
	} __xdata MIDIdevice;

	struct
	{
		unsigned long idVendorL;
		unsigned long idVendorH;
		unsigned long idProductL;
		unsigned long idProductH;
	} __xdata VendorProductID;

	void resetHubDevices()
	{
		VendorProductID.idVendorL = 0;
		VendorProductID.idVendorH = 0;
		VendorProductID.idProductL = 0;
		VendorProductID.idProductH = 0;
		MIDIdevice.connected = 0;
		MIDIdevice.interface = 0;
		MIDIdevice.inEndPoint = 0;
		MIDIdevice.outEndPoint = 0;
		MIDIdevice.type = 0;
	}

	void pollMIDIdevice()
	{
		__xdata unsigned char s, len;
		if (MIDIdevice.connected)
		{
			selectHubPort();
			//Recevive MIDI data from USB and Send MIDI data to UART
			s = hostTransfer(USB_PID_IN << 4 | MIDIdevice.inEndPoint & 0x7F, MIDIdevice.inEndPoint & 0x80 ? bUH_R_TOG | bUH_T_TOG : 0, 0);
			if (s == ERR_SUCCESS)
			{
				MIDIdevice.inEndPoint ^= 0x80;
				len = USB_RX_LEN;
				if (len)
				{
					parseUSBMIDIPacket(RxBuffer,len);
				}
			}

			//Receive MIDI data from UART and Send MIDI data from USB
			len = UART0Available();
			if(len){
				//Parse MIDI data. Note that parseUARTMIDIByte does not always make a MIDI events
				unsigned char t;
				for(t = 0;t < len;t++){
					parseUARTMIDIByte(UART0Receive());
				}
				//If there are MIDI events to send in TxBuffer
				if(midi_evt_count){
					//Fill the unused bytes by zero
					for(t = midi_evt_count * 4;t < MAX_PACKET_SIZE;t++)TxBuffer[t] = 0x00;
					for(t = 0;t < midi_evt_count * 4;t++){
						char b[3];
						puts(u8ToHEX(TxBuffer[t],b));
					}
					putchar('\n');
					UH_TX_LEN = midi_evt_count * 4;
					s = hostTransfer(USB_PID_OUT << 4 | MIDIdevice.outEndPoint & 0x7F, MIDIdevice.outEndPoint & 0x80 ? bUH_R_TOG | bUH_T_TOG : 0, 100);
					if(s == ERR_SUCCESS){
						MIDIdevice.outEndPoint ^= 0x80;
					}
					midi_evt_count = 0;
				}
			}
		}
	}

	unsigned char initializeRootHubConnection()
	{
		unsigned char retry, i, s = ERR_SUCCESS, cfg, dv_cls, addr;

		for (retry = 0; retry < 10; retry++) // todo test fewer retries
		{
			delay(100);
			delay(100); // todo test lower delay
			resetHubDevices();
			resetRootHubPort();
			for (i = 0; i < 100; i++) // todo test fewer retries
			{
				delay(1);
				if (enableRootHubPort() == ERR_SUCCESS)
					break;
			}
			if (i == 100)
			{
				puts("TIMEOUT\n");
				disableRootHubPort();
				continue;
			}

			selectHubPort();
			s = getDeviceDescriptor();
			puts("getDevDesc:");
			putchar((char)s + '0');
			puts("\n");
			if (s == ERR_SUCCESS)
			{
				dv_cls = ((PXUSB_DEV_DESCR)receiveDataBuffer)->bDeviceClass;
				VendorProductID.idVendorL = ((PXUSB_DEV_DESCR)receiveDataBuffer)->idVendorL;
				VendorProductID.idVendorH = ((PXUSB_DEV_DESCR)receiveDataBuffer)->idVendorH;
				VendorProductID.idProductL = ((PXUSB_DEV_DESCR)receiveDataBuffer)->idProductL;
				VendorProductID.idProductH = ((PXUSB_DEV_DESCR)receiveDataBuffer)->idProductH;
				addr = ((PUSB_SETUP_REQ)SetUSBAddressRequest)->wValueL; // todo wValue always 2.. does another id work?
				s = setUsbAddress(addr);
				puts("setUSBAddr:");
				putchar((char)s + '0');
				puts("\n");
				if (s == ERR_SUCCESS)
				{
					rootHubDevice.address = addr;
					s = getDeviceString();
					s = getConfigurationDescriptor();
					if (s == ERR_SUCCESS)
					{
						unsigned short i, total;
						unsigned char __xdata temp[512];
						PXUSB_ITF_DESCR currentInterface = 0;
						int interfaces;

						cfg = ((PXUSB_CFG_DESCR)receiveDataBuffer)->bConfigurationValue;

						interfaces = ((PXUSB_CFG_DESCR_LONG)receiveDataBuffer)->cfg_descr.bNumInterfaces;

						s = setUsbConfig(cfg);
						// parse descriptors
						total = ((PXUSB_CFG_DESCR)receiveDataBuffer)->wTotalLengthL + (((PXUSB_CFG_DESCR)receiveDataBuffer)->wTotalLengthH << 8);
						for (i = 0; i < total; i++)
							temp[i] = receiveDataBuffer[i];
						i = ((PXUSB_CFG_DESCR)receiveDataBuffer)->bLength;
						while (i < total)
						{
							unsigned char __xdata *desc = &(temp[i]);
							switch (desc[1])
							{
							case USB_DESCR_TYP_INTERF:
								currentInterface = ((PXUSB_ITF_DESCR)desc);
								break;
							case USB_DESCR_TYP_ENDP:
								puts("EP_DESC found\n");
								if (currentInterface->bInterfaceClass == USB_DEV_CLASS_AUDIO &&
									currentInterface->bInterfaceSubClass == USB_SUB_CLASS_MIDI)
								{
									puts("MIDI Device found\n");
									PXUSB_ENDP_DESCR d = (PXUSB_ENDP_DESCR)desc;
									if(d->bEndpointAddress & 0x80)
									{ // IN EP?
										MIDIdevice.inEndPoint = d->bEndpointAddress;
										MIDIdevice.connected = 1;
										MIDIdevice.interface = currentInterface->bInterfaceNumber;
									}
									else if(d->bmAttributes == 0x02){//Bulk OUT EP?
										MIDIdevice.outEndPoint = d->bEndpointAddress;
									}
								}
								break;
							case USB_DESCR_TYP_CS_INTF:
								break;
							case USB_DESCR_TYP_CS_ENDP:
								break;
							default:
								break;
							}
							i += desc[0];
						}
						return ERR_SUCCESS;
					}
				}
			}
			rootHubDevice.status = ROOT_DEVICE_FAILED;
			setUsbSpeed(1); // TODO define speeds
		}
		return s;
	}

	unsigned char checkRootHubConnections()
	{
		unsigned char s;
		s = ERR_SUCCESS;
		if (UIF_DETECT)
		{
			UIF_DETECT = 0;
			if (USB_MIS_ST & bUMS_DEV_ATTACH)
			{
				if (rootHubDevice.status == ROOT_DEVICE_DISCONNECT || (UHOST_CTRL & bUH_PORT_EN) == 0x00)
				{
					puts("USB Dev Attached\n");
					disableRootHubPort(); // todo really need to reset register?
					rootHubDevice.status = ROOT_DEVICE_CONNECTED;
					s = initializeRootHubConnection();
				}
			}
			else if (rootHubDevice.status >= ROOT_DEVICE_CONNECTED)
			{
				resetHubDevices();
				disableRootHubPort();
				s = ERR_USB_DISCON;
			}
		}
		return s;
	}