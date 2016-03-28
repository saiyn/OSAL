#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Mdc.h"
#include "Mdc_dio.h"



char * const DEF_DIO_CMD[SLOT_NUM][DIO_INPUT_N]={
    {"Coax1", "Coax2", "Optical1", "Optical2", "AESEBU"},
    {"Coax1", "Coax2", "Optical1", "Optical2", "AESEBU"}
};

char * const DEF_DIO_NAME[2][DIO_INPUT_N]=
{
    {"Coax1", "Coax2", "Optical1", "Optical2", "AES/EBU"},
    {"Coax1 ", "Coax2 ", "Optical1 ", "Optical2 ", "AES/EBU1 "}
};

uint8 AK4118_read(uint8 reg)
{
	uint8 addr, ret;
	
	addr = (0x20 & reg) << 1;
  addr |= (0x1F & reg);
	
	MDC_SPI_CS(0);
	MDC_SPI_PUTC(addr);
	ret = MDC_SPI_PUTC(0xff);
	MDC_SPI_CS(1);
	
	return ret;
}

int AK4118_write(uint8 reg, uint8 val)
{
	uint8 addr, ret;
	
	addr = (0x20 & reg) << 1;
  addr |= 0x20;
  addr |= (0x1F & reg);
	
	MDC_SPI_CS(0);
	MDC_SPI_PUTC(addr);
	MDC_SPI_PUTC(val);
	MDC_SPI_CS(1);
	
	bsp_delay_us(2);
	ret = AK4118_read(reg);
	
	if(val != ret){
		SYS_TRACE("reg 0x%02x write error: addr=0x%02x, read=0x%02x\r\n", addr, val, ret);
		return -1;
	}
	
	
	return 0;
}


void mdc_dio_init(mdc_slot_t slot)
{
	mdc_bus_select(slot);
	
	AK4118_write(0x00, 0x42);
	bsp_delay_us(10);
	AK4118_write(0x00, 0x43);
	bsp_delay_us(10);
	AK4118_write(0x01, 0x5a);
	AK4118_write(0x02, 0x80);
	AK4118_write(0x21, 0x02);
	AK4118_write(0x22, 0x00);
	
	mdc_bus_select(SLOT_NUM);
	
	SYS_TRACE("mdc_dio_init done\r\n");
}


void mdc_dio_input_select(mdc_slot_t slot, dio_input_t in)
{
	static uint8 table[DIO_INPUT_N]={5,4,3,1,0};
  uint8 input = table[in];

  mdc_bus_select(slot);

  
	AK4118_write(0x02, 0x80 | (input << 4));
	AK4118_write(0x03, 0x40 | input);
	
	
	mdc_bus_select(SLOT_NUM);
}


