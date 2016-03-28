#include "sys_head.h"
#include "OSAL_Soft_IIC.h"
#include "EPRom_driver.h"
#include "OSAL_Soft_IIC.h"
#include "OSAL_Console.h"

#define EPROM_PAGE_SIZE  16
#define EPROM_TOTAL_SIZE 2048

void epprom_sda(int level)
{
	 if(level){
		GPIOPinTypeGPIOInput(EPROM_SDA_PORT, EPROM_SDA_PIN);
		GPIO_PIN_SET(EPROM_SDA_PORT, EPROM_SDA_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(EPROM_SDA_PORT, EPROM_SDA_PIN);
		GPIO_PIN_SET(EPROM_SDA_PORT, EPROM_SDA_PIN, 0);
	}
}


void epprom_scl(int level)
{
	if(level){
		GPIOPinTypeGPIOInput(EPROM_SCL_PORT, EPROM_SCL_PIN);
		GPIO_PIN_SET(EPROM_SCL_PORT, EPROM_SCL_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(EPROM_SCL_PORT, EPROM_SCL_PIN);
		GPIO_PIN_SET(EPROM_SCL_PORT, EPROM_SCL_PIN, 0);
	}  
}


uint8 epprom_sda_in(void)
{
	 return (GPIO_ReadSinglePin(EPROM_SDA_PORT, EPROM_SDA_PIN));
}

int eeprom_write(iic_device_t device, uint32 addr, uint8 *buf, size_t size)
{
	uint8 write_size;
	size_t index = 0;
	
	OSAL_ASSERT(buf != NULL);
	
	do{
		if(size > EPROM_PAGE_SIZE){
			write_size = EPROM_PAGE_SIZE;
		}else{
			write_size = size;
		}
		if(osal_iic_write_buf(device, addr, buf, write_size) < 0){
			return -1;
		}
		buf += write_size;
		index += write_size;
		addr += write_size;
		bsp_delay_ms(50);
		
	}while(index < size);
	
	
	return size;
}

int eeprom_read(iic_device_t device, uint32 addr, uint8 *buf, size_t size)
{
	OSAL_ASSERT(buf != NULL);
	
	if(osal_iic_read_buf(device, addr, buf, size) < 0){
		return -1;
	}
	
	return size;
}

/*******************************For Test*********************************************/
int eep_write(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  int retval;
	
	retval = eeprom_write(EPROM, atoi(argv[1]), (uint8 *)argv[2], strlen(argv[2]));
	
	if(retval < 0){
		SYS_TRACE("eeprom write fail\r\n");
	}else{
		SYS_TRACE("eeprom write %d bytes success\r\n", retval);
	}
	
	return 0;
}

OSAL_CMD(eppw, 3, eep_write, "eeprom write test");


int eep_read(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  int retval;
	uint8 buf[64] = {0};
	
	retval = eeprom_read(EPROM, atoi(argv[1]), buf, atoi(argv[2]));
	
	if(retval < 0){
		SYS_TRACE("eeprom read fail\r\n");
	}else{
		SYS_TRACE("eeprom read=[%s]\r\n", buf);
	}
	
	return 0;
}

OSAL_CMD(eppr, 3, eep_read, "eeprom read test");







