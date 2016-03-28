/*
* Copyright (c) 2015, Hansong (Nanjing) Technology Ltd.
* All rights reserved.
* 
* ----File Info ----------------------------------------------- -----------------------------------------
* Name:     OSAL_Soft_IIC.c
* Author:   saiyn.chen
* Date:     2015/5/8
* Summary:  this file supply soft iic support.


* Change Logs:
* Date           Author         Notes
* 2015-05-8    saiyn.chen     first implementation
* 2015-05-28   saiyn.chen     change uint8 to uint32 in osal_iic_write_buf() and osal_iic_read_buf()
*                             to support the device whose reg map is 16bit or more
*/
#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Soft_IIC.h"
#include "bsp.h"
#include "pcm1795.h"
#include "Src4382.h"
#include "EPRom_driver.h"
#include "Mdc.h"


typedef void (*pIIcBusHandler)(int);
typedef uint8 (*pIIcAck)(void);

typedef struct{
	pIIcBusHandler sda;
	pIIcBusHandler scl;
	pIIcAck sda_in;
	uint8 address;
	uint8 map_size;
}iic_bus_t;



static iic_bus_t pcm1795 ={
	pcm1795_sda, pcm1795_scl,
	pcm1795_sda_in,PCM1795_IIC_ADDRESS,1
};

static iic_bus_t src4382 ={
	src4382_sda, src4382_scl,
	src4382_sda_in, SRC4382_IIC_ADDRESS,1
};

static iic_bus_t epprom ={
	epprom_sda, epprom_scl,
	epprom_sda_in,EPPROM_IIC_ADDRESS,2
};

static iic_bus_t mdc_slot1 = {
	mdc_slot1_sda, mdc_slot1_scl,
	mdc_slot1_sda_in, TEST_CARD_I2C_ADDR, 2
};

static iic_bus_t mdc_slot2 = {
	mdc_slot2_sda, mdc_slot2_scl,
	mdc_slot2_sda_in, TEST_CARD_I2C_ADDR, 2
};

/*manually register the iic device*/
static iic_bus_t* iic[]={
	&pcm1795, &src4382, &epprom,
	&mdc_slot1, &mdc_slot2,
};

#define SDA(b,x) iic[b]->sda(x)
#define SCL(b,x) iic[b]->scl(x)

static void iic_half_delay(void)
{
	int index = 0;
	
	while(index < IIC_HALF_DEALY_TIME){
		index++;
	}
}

static void iic_delay(void)
{
	 int index = 0;
	
	while(index < IIC_DELAY_TIME){
		index++;
	}
}


static void iic_start(iic_device_t device)
{
	 iic[device]->sda(1);
	 iic[device]->scl(1);
	 iic_half_delay();
	 iic[device]->sda(0);
	 iic_half_delay();
	 iic[device]->scl(0);
	 iic_half_delay();
}

static void iic_stop(iic_device_t device)
{
	 iic[device]->sda(0);
	 iic_half_delay();
	 iic[device]->scl(1);
	 iic_half_delay();
	 iic[device]->sda(1);
	 iic_half_delay();
}


static void iic_bit_out(iic_device_t device, uint8 bit)
{
	 iic[device]->sda(bit);
	 iic_half_delay();
	 iic[device]->scl(1);
	 iic_delay();
	 iic[device]->scl(0);
	 iic_delay();
}

static uint8 iic_bit_in(iic_device_t device) 
{
	 uint8 bit;
	
	 iic[device]->scl(1);
	 iic_delay();
	 bit = iic[device]->sda_in();
	 iic_delay();
	 iic[device]->scl(0);
	 iic_delay();
	
	 return bit;
}

static uint8 iic_byte_in(iic_device_t device, uint8 ack)
{
	 uint8 index,byte = 0;

	 iic[device]->sda(1);
	 iic_half_delay();
	 
   for(index = 0; index < 8; index++){
     byte <<= 1;
		 if(iic_bit_in(device)) byte |= 0x01;
	 }		 
	 
	 iic_bit_out(device, ack);
	 iic_half_delay();
	 iic[device]->sda(1);
	 iic_half_delay();
	 
	 return byte;
}

static uint8 iic_ack_in(iic_device_t device) 
{
	 uint8 bit;
	
	 iic[device]->sda(1);
	 iic_half_delay();
	
	 iic[device]->scl(1);
	 iic_delay();
	 iic_delay();
	 bit = iic[device]->sda_in();
	 iic_delay();
	 iic[device]->scl(0);
	 iic_delay();
	
	 return bit;
}


static int iic_byte_out(iic_device_t device, uint8 byte)
{
	  uint8 index;
	
	  for(index = 0; index < 8; index++){
			if(byte & 0x80){
				iic_bit_out(device,1);
			}else{
				iic_bit_out(device,0);
			}
			byte <<= 1;
		}
		
		if(iic_ack_in(device)){
			 SYS_TRACE("device[%d] no ack\r\n", device);
			 return -1;
		}
		
		return 0;
}

int osal_iic_write_buf(iic_device_t device, uint32 map, uint8 *buf, size_t cnt)
{
	  uint8 byte_out;
	
	  OSAL_ASSERT(device < IIC_DEVICE_NUM);
	  OSAL_ASSERT(buf != NULL);
	
	  iic_start(device);
	
	  byte_out = iic[device]->address & 0xfe;
	
	
	  if(iic_byte_out(device, byte_out) < 0){
			iic_stop(device);
			return -1;
		}

		
		if(iic[device]->map_size == 2){
			if(iic_byte_out(device, ((map >> 8) & 0xff)) < 0){
				iic_stop(device);
				return -1;
			}
			byte_out = map & 0xff;
			cnt += 1;
		}else if(iic[device]->map_size == 1){
			byte_out = map;
			cnt += 1;
		}
		
		while(cnt){
			if(iic_byte_out(device, byte_out) < 0){
				iic_stop(device);
				return -1;
			}
			
			byte_out = *buf++;
			cnt--; 
		}
		
		iic_stop(device);
		
		return 0;
}


int osal_iic_write_byte(iic_device_t device, uint32 map, uint8 data)
{
	  return(osal_iic_write_buf(device, map, &data, 1));
}


int osal_iic_read_buf(iic_device_t device, uint32 map, uint8 *buf, size_t cnt)
{
	uint8 byte_in;

  OSAL_ASSERT(buf != NULL);	
	OSAL_ASSERT(cnt > 0);
	
	 if(osal_iic_write_buf(device, map, (uint8 *)&map, 0) < 0){
		 return -1;
	 }
	 
	 iic_start(device);
	 
	 byte_in = iic[device]->address | 0x01;
	 
	 if(iic_byte_out(device, byte_in) < 0){
		 iic_stop(device);
		 return -1;
	 }
	 
	 while(cnt){
		 if(--cnt){
			 /*before read the last byte, we should send back the ack*/
			 byte_in = iic_byte_in(device, 0);
		 }else{
			 byte_in = iic_byte_in(device, 1);
		 }
		 *buf++ = byte_in;
	 }
	 
	 iic_stop(device);
	 
	 return 0;
}








