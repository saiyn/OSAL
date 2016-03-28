#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Mdc.h"
#include "Mdc_hdmi.h"

enum
{
    HDMI_MSG_ERR=0,
    HDMI_MSG_OK,
    HDMI_MSG_START,
    HDMI_MSG_END,
    HDMI_MSG_QUERY,
    HDMI_MSG_ERASE,
    HDMI_MSG_RUN_APP,
    HDMI_MSG_RECORD,
    HDMI_MSG_APP_OK,
    HDMI_MSG_APP_BAD
};


enum
{
    CMD_PORTA=0,
    CMD_PORTB,
    CMD_PORTC,
    CMD_PORTD,
    CMD_PORTN,
    CMD_GET_CLK,
    CMD_RESET_ON,
    CMD_RESET_OFF,
    CMD_VERSION,
    CMD_QUERY
};    

char * const DEF_HDMI_CMD[SLOT_NUM][HDMI_INPUT_N]=
{
    {"HDMI1", "HDMI2", "HDMI3"},
    {"HDMI1", "HDMI2", "HDMI3"}
};

char * const DEF_HDMI_NAME[2][HDMI_INPUT_N]=
{
    {"HDMI 1", "HDMI 2", "HDMI 3"},
    {"HDMI 1 ", "HDMI 2 ", "HDMI 3 "}
};

#define HDMI_IS_BUSY(slot)   mdc_int_get(slot)==0

static uint8 hdmi_write(mdc_slot_t slot, uint8 val)
{
	uint16 i = 0;
	uint8 output;
	
	MDC_SPI_CS(0);
	
	while(HDMI_IS_BUSY(slot)){
		i++;
		if(i > 0xf000){
			SYS_TRACE("mdc[%d] hdmi write timeout\r\n", slot);
		}
	}
	
	output = MDC_SPI_PUTC(val);
	
	MDC_SPI_CS(1);
	
	return output;
}


void hdmi_init(mdc_slot_t slot)
{
	uint8 ret;
	spi_ctl_t config = {8, 2, 400000};
	
	SYS_TRACE("SLOT[%d] hdmi card init\r\n", slot);
	
	mdc_bus_select(slot);
	
  bsp_spi0_ctl(&config);
	
	hdmi_write(slot, HDMI_MSG_END);
	hdmi_write(slot, HDMI_MSG_START);
	hdmi_write(slot, HDMI_MSG_RUN_APP);
	ret = hdmi_write(slot, HDMI_MSG_QUERY);
	if(ret != HDMI_MSG_APP_OK){
		SYS_TRACE("mdc %d app error %d \r\n", slot, ret);
	}
	
	mdc_bus_select(SLOT_NUM);
	
	config.protocal = 3;
	bsp_spi0_ctl(&config);
}


void hdmi_version_get(mdc_slot_t slot, version_t *ver)
{
	spi_ctl_t config = {8, 2, 400000};
	
	mdc_bus_select(slot);
	
	bsp_spi0_ctl(&config);
	
	hdmi_write(slot, CMD_VERSION);
	ver->major = hdmi_write(slot, CMD_QUERY);
	ver->minor = hdmi_write(slot, CMD_QUERY);
	ver->beta = hdmi_write(slot, CMD_QUERY);
	
	SYS_TRACE("mdc %d ver=v%d.%d%d \r\n", slot, ver->major, ver->minor, ver->beta);
	
	mdc_bus_select(SLOT_NUM);

  config.protocal = 3;
	bsp_spi0_ctl(&config);
}


void hdmi_input_select(mdc_slot_t slot, hdmi_input_t input)
{
	uint8 cmd;
	spi_ctl_t config = {8, 2, 400000};
	
	mdc_bus_select(slot);
	
	switch(input){
		
		case HDMI_INPUT_1:
			cmd = CMD_PORTC;
			break;
		
		case HDMI_INPUT_2:
			cmd = CMD_PORTB;
			break;
		
		case HDMI_INPUT_3:
			cmd = CMD_PORTA;
			break;
	
		
		default:
			cmd = CMD_PORTN;
			break;
		
	}
	
	bsp_spi0_ctl(&config);
	
	hdmi_write(slot, cmd);
	
	mdc_bus_select(SLOT_NUM);
	
	config.protocal = 3;
	bsp_spi0_ctl(&config);
}






