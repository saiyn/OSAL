//*****************************************************************************
//
// usb_stick_update.c - Example to update flash from a USB memory stick.
//
// Copyright (c) 2013 Texas Instruments Incorporated.  All rights reserved.
// Software License Agreement
// 
// Texas Instruments (TI) is supplying this software for use solely and
// exclusively on TI's microcontroller products. The software is owned by
// TI and/or its suppliers, and is protected under applicable copyright
// laws. You may not combine this software with "viral" open-source
// software in order to form a larger program.
// 
// THIS SOFTWARE IS PROVIDED "AS IS" AND WITH ALL FAULTS.
// NO WARRANTIES, WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT
// NOT LIMITED TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. TI SHALL NOT, UNDER ANY
// CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
// DAMAGES, FOR ANY REASON WHATSOEVER.
// 
// This is part of revision 2.0.1.11577 of the DK-TM4C129X Firmware Package.
//
//*****************************************************************************

//#include <stdint.h>
//#include <string.h>
//#include <stdbool.h>
//#include "inc/hw_flash.h"
//#include "inc/hw_gpio.h"
//#include "inc/hw_memmap.h"
//#include "inc/hw_nvic.h"
//#include "inc/hw_sysctl.h"
//#include "inc/hw_types.h"
//#include "driverlib/gpio.h"
//#include "driverlib/pin_map.h"
//#include "driverlib/rom.h"
//#include "driverlib/sysctl.h"
//#include "driverlib/udma.h"
#include "sys_head.h"
#include "usblib/usblib.h"
#include "usblib/usbmsc.h"
#include "usblib/host/usbhost.h"
#include "usblib/host/usbhmsc.h"
#include "simple_fs.h"
#include "OSAL_Task.h"
#include "OSAL.h"
#include "OSAL_Timer.h"
#include "lcd_driver.h"
#include "Mdc.h"
#include "System_Task.h"
#include "Display_Task.h"

mdc_slot_t gSlot;

uint16 gusb_crc;

#define MAX_RECORD_LENGTH 						256
#define USB_BLOCK_SIZE                512

#define USB_BASE_FLASH_ADDRESS        0x80001000
#define USB_MAX_FLASH_ADDRESS        0xF0000000

uint32 gBlockAddr;


static uint8 gBlockBuffer[USB_BLOCK_SIZE];

typedef enum
{
    RECORD_TYPE_DATA_RECORD = 0x00,
    RECORD_TYPE_EOF = 0x01,
    RECORD_TYPE_EXTENDED_ADDRESS = 0x04
} RECORD_TYPE;

typedef struct
{
    unsigned char       RecordLength;   // Length record data payload (adjusted
    unsigned int        LoadOffset;     // 16-bit offset to which the data will 
    RECORD_TYPE         RecordType;     // Type of data in the record
    unsigned char       data[MAX_RECORD_LENGTH] __attribute__ ((aligned (4)));      
                        // Record data buffer - needs to be 32-bit aligned so
                        //   that we can read 32-bit words out of it.
    unsigned char       Checksum;       // Checksum of the record

} RECORD_STRUCT; // hexadecimal format data for transfer to aggregator

// Stores the information about the current record
RECORD_STRUCT   current_record;

typedef enum
{
    RECORD_START_TOKEN = 0,
    RECORD_BYTE_COUNT_NIBBLE_1,
    RECORD_BYTE_COUNT_NIBBLE_0,
    RECORD_ADDRESS_NIBBLE_3,
    RECORD_ADDRESS_NIBBLE_2,
    RECORD_ADDRESS_NIBBLE_1,
    RECORD_ADDRESS_NIBBLE_0,
    RECORD_TYPE_NIBBLE_1,
    RECORD_TYPE_NIBBLE_0,
    RECORD_DATA,
    RECORD_CHECKSUM_NIBBLE_1,
    RECORD_CHECKSUM_NIBBLE_0 
} RECORD_STATE;

static RECORD_STATE    record_state;

extern uint8 gUpgradeTaskID;

static int do_hex_decode_and_upgrade(uint8 *buf, uint32 size, uint16 *crc);
uint8 AsciiToHexNibble(uint8 data);

static int usb_send_block(uint32 address, uint16 *crc);

#define AsciiToHexByte(m,l) ( (AsciiToHexNibble(m) << 4 ) | AsciiToHexNibble(l) )

//*****************************************************************************
//
// Defines the number of times to call to check if the attached device is
// ready.
//
//*****************************************************************************
#define USBMSC_DRIVE_RETRY      4

//*****************************************************************************
//
// The name of the binary firmware file on the USB stick.  This is the user
// application that will be searched for and loaded into flash if it is found.
// Note that the name of the file must be 11 characters total, 8 for the base
// name and 3 for the extension.  If the actual file name has fewer characters
// then it must be padded with spaces.  This macro should not contain the dot
// "." for the extension.
//
// Examples: firmware.bin --> "FIRMWAREBIN"
//           myfile.bn    --> "MYFILE  BN "
//
//*****************************************************************************
//#define USB_UPDATE_FILENAME "FIRMWAREBIN"
#define USB_UPDATE_FILENAME "C390U   HEX"
//*****************************************************************************
//
// The size of the flash for this microcontroller.
//
//*****************************************************************************
#define FLASH_SIZE (1 * 1024 * 1024)

//*****************************************************************************
//
// The starting address for the application that will be loaded into flash
// memory from the USB stick.  This address must be high enough to be above
// the USB stick updater, and must be on a 1K boundary.
// Note that the application that will be loaded must also be linked to run
// from this address.
//
//*****************************************************************************
#define APP_START_ADDRESS 0x8000

//*****************************************************************************
//
// A memory location and value that is used to indicate that the application
// wants to force an update.
//
//*****************************************************************************
#define FORCE_UPDATE_ADDR 0x20004000
#define FORCE_UPDATE_VALUE 0x1234cdef

//*****************************************************************************
//
// The prototype for the function that is used to call the user application.
//
//*****************************************************************************
void CallApplication(uint_fast32_t ui32StartAddr);

//*****************************************************************************
//
// The control table used by the uDMA controller.  This table must be aligned
// to a 1024 byte boundary.  In this application uDMA is only used for USB,
// so only the first 6 channels are needed.
//
//*****************************************************************************
#if defined(ewarm)
#pragma data_alignment=1024
tDMAControlTable g_sDMAControlTable[6];
#elif defined(ccs)
#pragma DATA_ALIGN(g_sDMAControlTable, 1024)
tDMAControlTable g_sDMAControlTable[6];
#else
tDMAControlTable g_sDMAControlTable[6] __attribute__ ((aligned(1024)));
#endif

//*****************************************************************************
//
// The global that holds all of the host drivers in use in the application.
// In this case, only the MSC class is loaded.
//
//*****************************************************************************
static tUSBHostClassDriver const * const g_ppHostClassDrivers[] =
{
    &g_sUSBHostMSCClassDriver
};

//*****************************************************************************
//
// This global holds the number of class drivers in the g_ppHostClassDrivers
// list.
//
//*****************************************************************************
#define NUM_CLASS_DRIVERS       (sizeof(g_ppHostClassDrivers) /               \
                                 sizeof(g_ppHostClassDrivers[0]))

//*****************************************************************************
//
// Hold the current state for the application.
//
//*****************************************************************************
volatile enum
{
    //
    // No device is present.
    //
    STATE_NO_DEVICE,

    //
    // Mass storage device is being enumerated.
    //
    STATE_DEVICE_ENUM,
}
g_eState;

//*****************************************************************************
//
// The instance data for the MSC driver.
//
//*****************************************************************************
tUSBHMSCInstance *g_psMSCInstance = 0;

//*****************************************************************************
//
// The size of the host controller's memory pool in bytes.
//
//*****************************************************************************
#define HCD_MEMORY_SIZE         128

//*****************************************************************************
//
// The memory pool to provide to the Host controller driver.
//
//*****************************************************************************
uint8_t g_pHCDPool[HCD_MEMORY_SIZE];

//*****************************************************************************
//
// A buffer for holding sectors read from the storage device
//
//*****************************************************************************
static uint8_t g_ui8SectorBuf[512];

//*****************************************************************************
//
// Global to hold the clock rate. Set once read many.
//
//*****************************************************************************
uint32_t g_ui32SysClock;

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Read a sector from the USB mass storage device.
//
// \param ui32Sector is the sector to read from the connected USB mass storage
// device (memory stick)
// \param pui8Buf is a pointer to the buffer where the sector data should be
// stored
//
// This is the application-specific implementation of a function to read
// sectors from a storage device, in this case a USB mass storage device.
// This function is called from the \e simple_fs.c file when it needs to read
// data from the storage device.
//
// \return Non-zero if data was read from the device, 0 if no data was read.
//
//*****************************************************************************
uint32_t
SimpleFsReadMediaSector(uint_fast32_t ui32Sector, uint8_t *pui8Buf)
{
    //
    // Return the requested sector from the connected USB mass storage
    // device.
    //
    return(USBHMSCBlockRead(g_psMSCInstance, ui32Sector, pui8Buf, 1));
}

//*****************************************************************************
//
// This is the callback from the MSC driver.
//
// \param ui32Instance is the driver instance which is needed when communicating
// with the driver.
// \param ui32Event is one of the events defined by the driver.
// \param pvData is a pointer to data passed into the initial call to register
// the callback.
//
// This function handles callback events from the MSC driver.  The only events
// currently handled are the \b MSC_EVENT_OPEN and \b MSC_EVENT_CLOSE.  This
// allows the main routine to know when an MSC device has been detected and
// enumerated and when an MSC device has been removed from the system.
//
// \return Returns \e true on success or \e false on failure.
//
//*****************************************************************************
static void
MSCCallback(tUSBHMSCInstance *psMSCInstance, uint32_t ui32Event, void *pvData)
{
    //
    // Determine the event.
    //
    switch(ui32Event)
    {
        //
        // Called when the device driver has successfully enumerated an MSC
        // device.
        //
        case MSC_EVENT_OPEN:
        {
            //
            // Proceed to the enumeration state.
            //
            g_eState = STATE_DEVICE_ENUM;
            break;
        }

        //
        // Called when the device driver has been unloaded due to error or
        // the device is no longer present.
        //
        case MSC_EVENT_CLOSE:
        {
            //
            // Go back to the "no device" state and wait for a new connection.
            //
            g_eState = STATE_NO_DEVICE;
            break;
        }

        default:
        {
            break;
        }
    }
}

extern void dis_simple_print(uint8 line, char *fmt, ...);


//*****************************************************************************
//
// Read the application image from the file system and program it into flash.
//
// This function will attempt to open and read the firmware image file from
// the mass storage device.  If the file is found it will be programmed into
// flash.  The name of the file to be read is configured by the macro
// \b USB_UPDATE_FILENAME.  It will be programmed into flash starting at the
// address specified by APP_START_ADDRESS.
//
// \return Zero if successful or non-zero if the file cannot be read or
// programmed.
//
//*****************************************************************************
uint32_t
ReadAppAndProgram(void)
{
    uint_fast32_t ui32FlashEnd;
    uint_fast32_t ui32FileSize;
    uint_fast32_t ui32DataSize;
    uint_fast32_t ui32Remaining;
    uint_fast32_t ui32ProgAddr;
    uint_fast32_t ui32SavedRegs[2];
    volatile uint_fast32_t ui32Idx;
    uint_fast32_t ui32DriveTimeout;

    //
    // Initialize the drive timeout.
    //
    ui32DriveTimeout = USBMSC_DRIVE_RETRY;

    //
    // Check to see if the mass storage device is ready.  Some large drives
    // take a while to be ready, so retry until they are ready.
    //
    while(USBHMSCDriveReady(g_psMSCInstance))
    {
        //
        // Wait about 500ms before attempting to check if the
        // device is ready again.
        //
        SysCtlDelay(g_ui32SysClock/(3*2));

        //
        // Decrement the retry count.
        //
        ui32DriveTimeout--;

        //
        // If the timeout is hit then return with a failure.
        //
        if(ui32DriveTimeout == 0)
        {
            return(1);
        }
    }

    //
    // Initialize the file system and return if error.
    //
    if(SimpleFsInit(g_ui8SectorBuf))
    {
        return(1);
    }

    //
    // Attempt to open the firmware file, retrieving the file/image size.
    // A file size of error means the file was not there, or there was an
    // error.
    //
    ui32FileSize = SimpleFsOpen(USB_UPDATE_FILENAME);
		
		dis_simple_print(LCD_LINE_1, "MDC USB Upgrade");
		
    if(ui32FileSize == 0)
    {
			  SYS_TRACE("SimpleFsOpen FAIL\r\n");
			  dis_simple_print(LCD_LINE_2, "open file fail");
        return(1);
    }
		
		SYS_TRACE("SimpleFsOpen Success:%d\r\n", ui32FileSize);
		mdc_usb_erase_flash(gSlot);
     
		dis_simple_print(LCD_LINE_2, "Erase Finished");
		bsp_delay_ms(1000);
		
		gBlockAddr = USB_BASE_FLASH_ADDRESS;
		gusb_crc = 0xffff;
    //
    // Get the size of flash.  This is the ending address of the flash.
    // If reserved space is configured, then the ending address is reduced
    // by the amount of the reserved block.
    //
    ui32FlashEnd = FLASH_SIZE;
#ifdef FLASH_RSVD_SPACE
    ui32FlashEnd -= FLASH_RSVD_SPACE;
#endif

    //
    // If flash code protection is not used, then change the ending address
    // to be the ending of the application image.  This will be the highest
    // flash page that will be erased and so only the necessary pages will
    // be erased.  If flash code protection is enabled, then all of the
    // application area pages will be erased.
    //
#ifndef FLASH_CODE_PROTECTION
    ui32FlashEnd = ui32FileSize + APP_START_ADDRESS;
#endif

    //
    // Check to make sure the file size is not too large to fit in the flash.
    // If it is too large, then return an error.
    //
//    if((ui32FileSize + APP_START_ADDRESS) > ui32FlashEnd)
//    {
//        return(1);
//    }

    //
    // Enter a loop to erase all the requested flash pages beginning at the
    // application start address (above the USB stick updater).
    //
    for(ui32Idx = APP_START_ADDRESS; ui32Idx < ui32FlashEnd; ui32Idx += 1024)
    {
        //ROM_FlashErase(ui32Idx);
    }

    //
    // Enter a loop to read sectors from the application image file and
    // program into flash.  Start at the user app start address (above the USB
    // stick updater).
    //
    ui32ProgAddr = APP_START_ADDRESS;
    ui32Remaining = ui32FileSize;
    while(SimpleFsReadFileSector())
    {
        //
        // Compute how much data was read from this sector and adjust the
        // remaining amount.
        //
        ui32DataSize = ui32Remaining >= 512 ? 512 : ui32Remaining;
        ui32Remaining -= ui32DataSize;

			  SYS_TRACE("SimpleFsReadFileSector:ui32Remaining:[%d]\r\n", ui32Remaining);
			
			  dis_simple_print(LCD_LINE_2, "Updating... %d%%", (ui32FileSize - ui32Remaining)*100/ui32FileSize);
			
			  do_hex_decode_and_upgrade(g_ui8SectorBuf, ui32DataSize, &gusb_crc);
		
        //
        // If there is more image to program, then update the programming
        // address.  Progress will continue to the next iteration of
        // the while loop.
        //
        if(ui32Remaining)
        {
            ui32ProgAddr += ui32DataSize;
        }

        //
        // Otherwise we are done programming so perform final steps.
        // Program the first two words of the image that were saved earlier,
        // and return a success indication.
        //
        else
        {
            /****finished*****/
					  usb_send_block(gBlockAddr, &gusb_crc);
					  mdc_usb_finish_program(gSlot, gusb_crc);
					  usb_bl_check_flash_status(gSlot);
            return(0);
        }
    }

    //
    // If we make it here, that means that an attempt to read a sector of
    // data from the device was not successful.  That means that the complete
    // user app has not been programmed into flash, so just return an error
    // indication.
    //
    return(1);
}

//*****************************************************************************
//
// This is the main routine for performing an update from a mass storage
// device.
//
// This function forms the main loop of the USB stick updater.  It polls for
// a USB mass storage device to be connected,  Once a device is connected
// it will attempt to read a firmware image from the device and load it into
// flash.
//
// \return None.
//
//*****************************************************************************
void
UpdaterUSB(void)
{
  static uint32 cnt = 0;
	
        USBHCDMain();

        //
        // Check for a state change from the USB driver.
        //
        switch(g_eState)
        {
            //
            // This state means that a mass storage device has been
            // plugged in and enumerated.
            //
            case STATE_DEVICE_ENUM:
            {
                //
                // Attempt to read the application image from the storage
                // device and load it into flash memory.
                //
                if(ReadAppAndProgram())
                {
                    //
                    // There was some error reading or programming the app,
                    // so reset the state to no device which will cause a
                    // wait for a new device to be plugged in.
                    //
                    g_eState = STATE_NO_DEVICE;
									  bsp_delay_ms(2000);
										//osal_start_timerEx(gUpgradeTaskID, UPGRADE_POLL_SMG, 100);
									  dis_simple_print(LCD_LINE_1, "USB Upgrade Fail");
									  dis_simple_print(LCD_LINE_2, "Pls Cycle Power");
                }
                else
                {
                    //
                    // User app load and programming was successful, so reboot
                    // the micro.  Perform a software reset request.  This
                    // will cause the microcontroller to reset; no further
                    // code will be executed.
                    //
									
									SYS_TRACE("usb upgrade done\r\n");
									
									bsp_delay_ms(2000);
									
									dis_simple_print(LCD_LINE_1, "USB Upgrade Done");
									dis_simple_print(LCD_LINE_2, "Pls Cycle Power");
									
//									mdc_power_set(gSlot, OFF);
//		
//									bsp_delay_ms(100);
//		
//									mdc_power_set(gSlot, ON);
//									
//									display_menu_jump(SOURCE_MENU);
//		              Send_DisplayTask_Msg(DIS_UPDATE, 0);	
									
									
                }
                break;
            }

            //
            // This state means that there is no device present, so just
            // do nothing until something is plugged in.
            //
            case STATE_NO_DEVICE:
            {
							
							SYS_TRACE("STATE_NO_DEVICE\r\n");
							osal_start_timerEx(gUpgradeTaskID, UPGRADE_POLL_SMG, 100);
							
							if(cnt++ > MDC_USB_UPGRADE_TIMEOUT){
								dis_simple_print(LCD_LINE_1, "No USB Found");
								dis_simple_print(LCD_LINE_2, "Pls insert USB");
							}
							
							
              break;
            }
        }
}

//*****************************************************************************
//
// Configure the USB controller and power the bus.
//
// This function configures the USB controller for host operation.
// It is assumed that the main system clock has been configured at this point.
//
// \return None.
//
//*****************************************************************************
void
ConfigureUSBInterface(void)
{
    //
    // Enable the uDMA controller and set up the control table base.
    // This is required by usblib.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_UDMA);
    SysCtlDelay(80);
    uDMAEnable();
    uDMAControlBaseSet(g_sDMAControlTable);

    //
    // Enable the USB controller.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_USB0);

    //
    // Set the USB pins to be controlled by the USB controller.
    //
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
    GPIOPinConfigure(GPIO_PD6_USB0EPEN);
    GPIOPinTypeUSBDigital(GPIO_PORTD_BASE, GPIO_PIN_6);
    SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
    GPIOPinTypeUSBAnalog(GPIO_PORTL_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    GPIOPinTypeUSBAnalog(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);

    //
    // Register the host class driver
    //
    USBHCDRegisterDrivers(0, g_ppHostClassDrivers, NUM_CLASS_DRIVERS);

    //
    // Open an instance of the mass storage class driver.
    //
    g_psMSCInstance = USBHMSCDriveOpen(0, MSCCallback);

    //
    // Initialize the power configuration. This sets the power enable signal
    // to be active high and does not enable the power fault.
    //
    USBHCDPowerConfigInit(0, USBHCD_VBUS_AUTO_HIGH | USBHCD_VBUS_FILTER);

    //
    // Force the USB mode to host with no callback on mode changes since
    // there should not be any.
    //
    USBStackModeSet(0, eUSBModeForceHost, 0);

    //
    // Wait 10ms for the pin to go low.
    //
    SysCtlDelay(gSysClock/100);

    //
    // Initialize the host controller.
    //
    USBHCDInit(0, g_pHCDPool, HCD_MEMORY_SIZE);
}

//*****************************************************************************
//
// Generic configuration is handled in this function.
//
// This function is called by the start up code to perform any configuration
// necessary before calling the update routine.  It is responsible for setting
// the system clock to the expected rate and setting flash programming
// parameters prior to calling ConfigureUSBInterface() to set up the USB
// hardware.
//
// \return None.
//
//*****************************************************************************
void
UpdaterMain(void)
{
    //
    // Make sure NVIC points at the correct vector table.
    //
//    HWREG(NVIC_VTABLE) = 0; 

    //
    // Run from the PLL at 120 MHz.
    //
//    g_ui32SysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ |
//                                       SYSCTL_OSC_MAIN |
//                                       SYSCTL_USE_PLL |
//                                       SYSCTL_CFG_VCO_480), 120000000);

    //
    // Enable lazy stacking for interrupt handlers.  This allows floating-point
    // instructions to be used within interrupt handlers, but at the expense of
    // extra stack usage.
    //
    FPULazyStackingEnable();

    //
    // Configure the USB interface and power the bus.
    //
    ConfigureUSBInterface();

    //
    // Call the updater function.  This will attempt to load a new image
    // into flash from a USB memory stick.
    //
    //UpdaterUSB();
}

/*********************Help Function******************************/

uint8 AsciiToHexNibble(uint8 data)
{
    if (data < '0')                     // return 0 for an invalid characters
    {
        return 0;
    }
    else if (data <= '9')               // handle numbers
    {
        return ( data - '0' );
    }
    else if (data < 'A')
    {
        return 0;
    }
    else if (data <= 'F')               // handle uppercase letters
    {
        return ( data - 'A' + 10 );
    }
    else if (data < 'a')
    {
        return 0;
    }
    else if (data <= 'f')               // handle lowercase letters
    {
        return ( data - 'a' + 10 );
    }
    else
    {
        return 0;
    }

} 

static uint32 _get_block_address(uint32 address)
{
	uint32 ret;
	
	if(address < USB_BASE_FLASH_ADDRESS){
		SYS_TRACE("USB: Addr < USB_BASE_FLASH_ADDRESS, 0x%08x\r\n", address);
		return USB_BASE_FLASH_ADDRESS;
	}
	
	ret = address / USB_BLOCK_SIZE;
	ret *= USB_BLOCK_SIZE;
	
	return ret;
}

static int usb_send_block(uint32 address, uint16 *crc)
{
	int retval = 0;
	DWORD_VAL  block_addr;
	
	block_addr.val = address;
	
	retval = mdc_usb_send_block(gSlot, block_addr, gBlockBuffer, USB_BLOCK_SIZE, crc);
	
	memset(gBlockBuffer, 0, USB_BLOCK_SIZE);
	
	return retval;
}

int USB_ProgramHexRecord(RECORD_STRUCT* record, UINT32 cur_address, uint16 *crc)
{
	uint32 i,j;
	uint8 *pdata;
	int retval = 0;
	
	if(cur_address >= (gBlockAddr + USB_BLOCK_SIZE)){
		retval = usb_send_block(gBlockAddr, crc);
		gBlockAddr = _get_block_address(cur_address);
	}
	
	/*above handler has make sure we won't overflow*/
	pdata = &gBlockBuffer[cur_address - gBlockAddr];
	
	for(i = 0; i < (*record).RecordLength; i++){
		*pdata = (*record).data[i];
		pdata++;
		if(pdata == gBlockBuffer + USB_BLOCK_SIZE){
			break;
		}
	}
	
	/*gBlockBuffer is full*/
	if(i < (*record).RecordLength){
		retval = usb_send_block(gBlockAddr, crc);
		gBlockAddr += USB_BLOCK_SIZE;
		i++;
		
		for(j = 0; i < (*record).RecordLength; i++, j++){
			gBlockBuffer[j] = (*record).data[i];
		}
	}
	
	
	return retval;
}


static int do_hex_decode_and_upgrade(uint8 *buf, uint32 size, uint16 *crc)
{
	BYTE *pdata =   (BYTE *)&buf[0];
	static WORD_VAL        byteCountASCII;
	static BYTE            byteEvenVsOdd = 0;
	static BYTE            recordDataCounter = 0;
	static DWORD_VAL       addressASCII;
	static WORD_VAL        recordTypeASCII;
	static WORD_VAL        checksumASCII;
  static WORD_VAL        dataByteASCII;
	static BYTE            calculated_checksum;
	static DWORD_VAL       totalAddress;
  static WORD_VAL        extendedAddress;
	uint16 j;
	
	while(size){
		switch(record_state){
			case RECORD_START_TOKEN:
				if(*pdata == ':'){
					record_state = RECORD_BYTE_COUNT_NIBBLE_1;
				}else{
					/*discard \r\n*/
					if(*pdata != 0x0d && *pdata != 0x0a){
						SYS_TRACE("USB: Start code expected but not found.\r\n");
						return -1;
					}
				}
				pdata++;
				break;
			
			case RECORD_BYTE_COUNT_NIBBLE_1:
				byteCountASCII.v[1] = *pdata++;
			  record_state = RECORD_BYTE_COUNT_NIBBLE_0;
				break;
			
			case RECORD_BYTE_COUNT_NIBBLE_0:
				byteCountASCII.v[0] = *pdata++;
			  current_record.RecordLength = AsciiToHexByte(byteCountASCII.v[1],byteCountASCII.v[0]);;
			  byteEvenVsOdd = 0;
        recordDataCounter = 0;
			  record_state = RECORD_ADDRESS_NIBBLE_3;
				break;
			
			case RECORD_ADDRESS_NIBBLE_3:
				addressASCII.v[3] = *pdata++;
			  record_state = RECORD_ADDRESS_NIBBLE_2;
				break;
			
			case RECORD_ADDRESS_NIBBLE_2:
				addressASCII.v[2] = *pdata++;
			  record_state = RECORD_ADDRESS_NIBBLE_1;
				break;
				
			case RECORD_ADDRESS_NIBBLE_1:
				addressASCII.v[1] = *pdata++;
			  record_state = RECORD_ADDRESS_NIBBLE_0;
				break;
			
			case RECORD_ADDRESS_NIBBLE_0:
				addressASCII.v[0] = *pdata++;
			  current_record.LoadOffset = ((AsciiToHexByte(addressASCII.v[3],addressASCII.v[2]))<<8) + AsciiToHexByte(addressASCII.v[1],addressASCII.v[0]);
			  record_state = RECORD_TYPE_NIBBLE_1;
				break;
			
			case RECORD_TYPE_NIBBLE_1:
				recordTypeASCII.v[1] = *pdata++;
			  record_state = RECORD_TYPE_NIBBLE_0;
				break;
			
			case RECORD_TYPE_NIBBLE_0:
				recordTypeASCII.v[0] = *pdata++;
        current_record.RecordType = AsciiToHexByte(recordTypeASCII.v[1],recordTypeASCII.v[0]);
			  if(current_record.RecordLength == 0){
					record_state = RECORD_CHECKSUM_NIBBLE_1;
				}else{
					record_state = RECORD_DATA;
				}
				break;
			
			case RECORD_DATA:
				if(byteEvenVsOdd == 0){
					dataByteASCII.v[1] = *pdata++;
					byteEvenVsOdd = 1;
				}else{
					dataByteASCII.v[0] = *pdata++;
					current_record.data[recordDataCounter++] = AsciiToHexByte(dataByteASCII.v[1],dataByteASCII.v[0]);
					byteEvenVsOdd = 0;
				}
				
				if(recordDataCounter == current_record.RecordLength){
					record_state = RECORD_CHECKSUM_NIBBLE_1;
				}
				
				break;
				
			case RECORD_CHECKSUM_NIBBLE_1:
				checksumASCII.v[1] = *pdata++;
			  record_state = RECORD_CHECKSUM_NIBBLE_0;
				break;
			
			case RECORD_CHECKSUM_NIBBLE_0:
				checksumASCII.v[0] = *pdata++;
			  current_record.Checksum = AsciiToHexByte(checksumASCII.v[1],checksumASCII.v[0]);
			  
			  calculated_checksum = current_record.RecordLength +
                                            (current_record.LoadOffset&0xFF) +
                                            ((current_record.LoadOffset>>8)&0xFF) +
                                            current_record.RecordType;
			
			   for(j = 0; j < current_record.RecordLength; j++){
					  calculated_checksum += current_record.data[j];
				 }
			
				 calculated_checksum ^= 0xff;
				 calculated_checksum += 1;
				 
				 if(calculated_checksum != current_record.Checksum){
					 SYS_TRACE("USB: Calculated checksum didn't match the hex record checksum.\r\n");
					 return -1;
				 }
				 
				 switch(current_record.RecordType){
					 
					 case RECORD_TYPE_DATA_RECORD:
						 totalAddress.word.HW = extendedAddress.val;
             totalAddress.word.LW = current_record.LoadOffset;
					   if(totalAddress.val < USB_BASE_FLASH_ADDRESS || totalAddress.val > USB_MAX_FLASH_ADDRESS){
							 SYS_TRACE("RECORD_TYPE_DATA_RECORD error\r\n");
							 break;
						 }
						 
						 if(USB_ProgramHexRecord(&current_record, totalAddress.val, crc) < 0){
							 SYS_TRACE("USB:There was an error programming the data\r\n");
							 return -1;
						 }
						 
						 break;
						 
						 
						case RECORD_TYPE_EOF:
							
							break;
						
						case RECORD_TYPE_EXTENDED_ADDRESS:
							extendedAddress.v[1] = current_record.data[0];
              extendedAddress.v[0] = current_record.data[1];
							break;
					 
				 }
				 
				 record_state = RECORD_START_TOKEN;
				 
				break;
			
			default:
				SYS_TRACE("USB: Loader state machine in an unknown state.\r\n");
				return -1;
		}
		
		size--;
	}
	
	
	return 0;
}




/****************OSAL SYSTERM INTERFACE***********************/

uint8 gUpgradeTaskID;


void UpgradeTaskInit(uint8 task_id)
{
	 gUpgradeTaskID = task_id;
}


uint16 UpgradeEventLoop(uint8 task_id, uint16 events)
{
	
	if(events & UPGRADE_POLL_SMG){
		UpdaterUSB();
		return (events ^ UPGRADE_POLL_SMG);
	}
	
	
	return 0;
}



