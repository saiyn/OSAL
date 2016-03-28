#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Mdc.h"
#include "Mdc_hdmi_v2.h"
#include "OSAL_RingBuf.h"
#include "System_Task.h"


char * const DEF_HDMI_V2_CMD[SLOT_NUM][HDMI_V2_INPUT_N]={
    {"HDMI1", "HDMI2", "HDMI3","HDMIARC"},
    {"HDMI1", "HDMI2", "HDMI3","HDMIARC"}
};


char * const DEF_HDMI_V2_NAME[2][HDMI_V2_INPUT_N]=
{
    {"HDMI 1", "HDMI 2", "HDMI 3","HDMI ARC"},
    {"HDMI 1 ", "HDMI 2 ", "HDMI 3 ","HDMI ARC "}
};


enum
{
    HDMI2_MSG_ERROR = 0,
    HDMI2_MSG_TIME_OUT,
    HDMI2_MSG_OK,
};



#define HDMI_IS_BUSY(slot)   mdc_int_get(slot)==0

static uint16 sloop = 0; 
static void hdmi_spi_shortDelay(uint16 a)
{
    sloop = a;
    while(sloop--);
}

static uint8 hdmi_try_write(mdc_slot_t slot, uint8 val)
{
	uint16 i = 0;
	uint8 output;
	
	MDC_SPI_CS(0);
	
	while(HDMI_IS_BUSY(slot)){
		i++;
		if(i > 0xf000){
			i = 0;
			SYS_TRACE("mdc[%d] hdmi write timeout\r\n", slot);
		}
	}
	
	output = MDC_SPI_PUTC(val);
	
	MDC_SPI_CS(1);

	
	return output;
}

static uint8 hdmi_write(mdc_slot_t slot, uint8 data)
{
	uint8 retval;

	OSAL_ASSERT(slot < SLOT_NUM);
	
	retval = bsp_spi_operate(SSI0_BASE, data);
	
	ring_buffer_write((ringbuf_device_t)(MDC_SLOT1+slot), (uint8)retval);
	

	return retval;
	
}

static uint8 hdmi_read(mdc_slot_t slot, uint8 data)
{
	return bsp_spi_operate(SSI0_BASE, data);
}

static void hdmiv2_get_byte(mdc_slot_t slot, uint8 *rx)
{
	if(Is_ring_buffer_empty((ringbuf_device_t)(MDC_SLOT1+slot))){
		MDC_SPI_CS(0);
		*rx = hdmi_read(slot, 0x00);
		MDC_SPI_CS(1);
	}else{
		*rx = ring_buffer_read((ringbuf_device_t)(MDC_SLOT1+slot));
	}
	
}


static uint8 hdmiv2_bl_get_msg(mdc_slot_t slot, uint8 *msg, uint8 len)
{
	uint16 i = 0;
	
	while(hdmi_try_write(slot, 0) != '#'){
		bsp_delay_ms(3);
		i++;
		if(i > 1000){
			i = 0;
			SYS_TRACE("\r\nWait HDMIV2 boot start \r\n");
			return 	HDMI2_MSG_TIME_OUT;
		}
	}
	
	for(i = 0; i < len; i++){
		msg[i] = hdmi_try_write(slot, 0);
	}
	
	i = hdmi_try_write(slot, 0);
	
	if(i != ';'){
		SYS_TRACE("hdmiv2_bl_get_msg error:%d\r\n", i);
		return HDMI2_MSG_ERROR;
	}
	
	return HDMI2_MSG_OK;
	
}


static uint8 hdmiv2_set_run_app(mdc_slot_t slot)
{
	uint8 status[4] = {0};
	uint8 ret;

	hdmi_try_write(slot, '#');
	hdmi_try_write(slot, HDMI2_CMD_RUN_APP);
	hdmi_try_write(slot, 0);
  hdmi_try_write(slot, ';');
	
	ret = hdmiv2_bl_get_msg(slot, status, 3);
	SYS_TRACE("hdmiv2_set_run_app: %x, %x, %x\r\n", status[0], status[1], status[2]);

  if(status[0] == HDMI2_CMD_RUN_APP && status[1] == 1 && status[2] == 1){
		ret = HDMI2_MSG_OK;
	}else{
		ret = HDMI2_MSG_ERROR;
	}
	
	
	return ret;
}

void hdmi_V2_init(mdc_slot_t slot)
{
	uint8 ret;
	
	SYS_TRACE("SLOT[%d] hdmi card init\r\n", slot);
	
	mdc_bus_select(slot);
	
	do{
		ret = hdmiv2_set_run_app(slot);
		if(ret != HDMI2_MSG_OK){
			SYS_TRACE("mdc %d app error %d \r\n", slot, ret);
			bsp_delay_ms(1000);
		}
		
	}while(ret != HDMI2_MSG_OK);
	
	mdc_bus_select(SLOT_NUM);
	
}

void hdmi_V2_version_get(mdc_slot_t slot, version_t *ver)
{
	uint8 status[8] = {0};
  uint8 ret;
	
	mdc_bus_select(slot);
	
  hdmi_try_write(slot, '#');
  hdmi_try_write(slot, HDMI2_CMD_GET_VERSION);
  hdmi_try_write(slot, 0);
	hdmi_try_write(slot, ';');

	ret = hdmiv2_bl_get_msg(slot, status, 5);
	
	if(status[0] == HDMI2_CMD_GET_VERSION && status[1] == 3){
		ver->major = status[2];
		ver->minor = status[3];
		ver->beta = status[4];
		
		SYS_TRACE("mdc %d ver=v%d.%d%d \r\n", slot, ver->major, ver->minor, ver->beta);
		ret = HDMI2_MSG_OK;
	}else{
		ret = HDMI2_MSG_ERROR;
		
		SYS_TRACE("hdmi_V2_version_get Fail..%d,%d,%d,%d\r\n", status[0], status[1], status[2], status[3]);
	}


	mdc_bus_select(SLOT_NUM);
}

void hdmi_v2_send_msg(mdc_slot_t slot, HDMI2_cmd_t cmd, uint8 *data, uint8 len)
{
	uint8 j = 0;
	
	mdc_bus_select(slot);
	
	MDC_SPI_CS(0);
	hdmi_write(slot, '#');
	MDC_SPI_CS(1);
	MDC_SPI_CS(0);
	hdmi_write(slot, cmd);
	MDC_SPI_CS(1);
	MDC_SPI_CS(0);
	hdmi_write(slot, len);
	MDC_SPI_CS(1);
	while(j < len){
		MDC_SPI_CS(0);
	  hdmi_write(slot, *(data + j));
	  MDC_SPI_CS(1);
		j++;
	}
	MDC_SPI_CS(0);
	hdmi_write(slot, ';');
	MDC_SPI_CS(1);
	
	mdc_bus_select(SLOT_NUM);
}


void hdmi_V2_input_select(mdc_slot_t slot, hdmi_v2_input_t input)
{
	uint8 cmd;
	
	mdc_bus_select(slot);
	
	MDC_SPI_CS(0);
	hdmi_write(slot, '#');
	MDC_SPI_CS(1);
  hdmi_spi_shortDelay(100);
	MDC_SPI_CS(0);
	hdmi_write(slot, HDMI2_CMD_INPUT_SEL);
	MDC_SPI_CS(1);
  hdmi_spi_shortDelay(100);
	MDC_SPI_CS(0);
	hdmi_write(slot, 1);
	MDC_SPI_CS(1);
  hdmi_spi_shortDelay(100);
	MDC_SPI_CS(0);
	hdmi_write(slot, input);
	MDC_SPI_CS(1);
  hdmi_spi_shortDelay(100);
	MDC_SPI_CS(0);
	hdmi_write(slot, ';');
	MDC_SPI_CS(1);
  hdmi_spi_shortDelay(100);

	
	mdc_bus_select(SLOT_NUM);
}

static void hdmiv2_volume_cmd_handler(mdc_slot_t slot, sys_state_t *sys, uint8 vol)
{
	int volume;
	static int last_volume = -1;
	
	if(vol > 100) return;
	 
	volume = ((vol / 100.0) * MASTER_VOL_WIDTH + (MASTER_VOL_INDEX_MIN_INT << 1)) / 2;
	
	if(volume <= MASTER_VOL_INDEX_MAX && volume != last_volume){
		sys->master_vol = volume;
		Send_SysTask_Msg(SYS_VOL_SET, 0);
		last_volume = volume;
	}
	
}

static void hdmiv2_mute_cmd_handler(mdc_slot_t slot, sys_state_t *sys, uint8 mute)
{
	
	if(mute == 1){
		sys->is_mute = true;
	}else{
		sys->is_mute = false;
	}
	
	Send_SysTask_Msg(SYS_VOL_SET, 0);
	
}

static void hdmiv2_msg_handler(mdc_slot_t slot, uint8 *msg)
{
	uint8 j;
	
	SYS_TRACE("HDMI MSG:");
	for(j =0; j < (msg[1] + 2); j++){
		SYS_TRACE("%2x ", msg[j]);
	}
	SYS_TRACE("\r\n");
	
	switch(msg[0]){
		case HDMI2_CMD_GET_VERSION:
			SYS_TRACE("HDMI2_CMD_GET_VERSION\r\n");
			break;
		
		case HDMI2_CMD_MUTE:
			SYS_TRACE("HDMI2_CMD_MUTE\r\n");
			hdmiv2_mute_cmd_handler(slot,gSystem_t, msg[2]);
			break;
		
		case HDMI2_CMD_INPUT_SEL:
			SYS_TRACE("HDMI2_CMD_INPUT_SEL\r\n");
			break;
		
		case HDMI2_CMD_VOLUME:
			SYS_TRACE("HDMI2_CMD_VOLUME\r\n");
		  hdmiv2_volume_cmd_handler(slot,gSystem_t, msg[2]);
			break;
		
		default:
			SYS_TRACE("hdmi invalid msg\r\n");
			break;
	}
	
	
	
}

static void hdmiv2_check_loop(mdc_slot_t slot)
{
	uint8 msg_buf[128] = {0};
	uint8 index;
  uint8 byte = 0;

	mdc_bus_select(slot);

	if(Is_ring_buffer_empty((ringbuf_device_t)(MDC_SLOT1+slot)) == true){
		MDC_SPI_CS(0);
		if(hdmi_read(slot, 0x00) != '#'){
			MDC_SPI_CS(1);
			goto ret;
		}else{
			SYS_TRACE("hdmiv2_check_loop: get #\r\n");
		}
		MDC_SPI_CS(1);
		MDC_SPI_CS(0);
		msg_buf[0] = hdmi_read(slot, 0x00);
		MDC_SPI_CS(1);
		MDC_SPI_CS(0);
		msg_buf[1] = hdmi_read(slot, 0x00);
		MDC_SPI_CS(1);
		if(msg_buf[1]){
			for(index = 0; index < msg_buf[1]; index++){
				MDC_SPI_CS(0);
				msg_buf[index + 2] = hdmi_read(slot, 0x00);
				MDC_SPI_CS(1);
			}
		}
		
	}else{
		while((byte = ring_buffer_read((ringbuf_device_t)(MDC_SLOT1+slot))) != '#'){
			if(Is_ring_buffer_empty((ringbuf_device_t)(MDC_SLOT1+slot)) == true){
				goto ret;
			}
		}
		
		hdmiv2_get_byte(slot, &byte);
		msg_buf[0] = byte;
		hdmiv2_get_byte(slot, &byte);
		msg_buf[1] = byte;
		if(msg_buf[1]){
			for(index = 0; index < msg_buf[1]; index++){
				hdmiv2_get_byte(slot, &byte);
				msg_buf[2 + index] = byte;
			}
		}
		
		
	}
	
	hdmiv2_msg_handler(slot, msg_buf);
	
ret:
		mdc_bus_select(SLOT_NUM);
	
	return;
}


void hdmi_v2_task(mdc_slot_t slot)
{
	hdmiv2_check_loop(slot);
	
	
}



















