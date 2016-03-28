#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Mdc.h"
#include "Mdc_usb.h"
#include "OSAL_RingBuf.h"
#include "System_Task.h"


#define USB_READ_TIMEOUT  1000

enum usb_task_state{
	USB_WAIT_START = 0,
	USB_WAIT_END,
	
};

char *const DEF_USB_CMD[SLOT_NUM][USB_INPUT_N] = {
    {"Computer",  "USBBack",  "USBLocal"},
    {"Computer",  "USBBack",  "USBLocal"},
};

char *const DEF_USB_NAME[2][USB_INPUT_N] = {
    {"Computer",   "USB Back",   "USB Local"},
    {"Computer ", "USB Back ", "USB Local "}
};

#define USB_PA            0
#define XMOS_PA            1

static uint8 CommandIndex[SLOT_NUM] = {0};
static uint8 CommandBuf[SLOT_NUM][MSG_MAX_LEN];


static uint8 sloop=0;

static void usb_spi_shortDelay(uint8 a)
{
    sloop = a;
    while(sloop--);
}

static void usb_enable_spi(void)
{
	mdc_pa_set(USB_PA);
  
	MDC_SPI_CS(0);
}

static void usb_disable_spi(void)
{
	MDC_SPI_CS(1);
}

static int usb_read(mdc_slot_t slot, uint16 data)
{
	int retval;
	
	OSAL_ASSERT(slot < SLOT_NUM);
	
	retval = bsp_spi_operate(SSI0_BASE, data);
	
	ring_buffer_write((ringbuf_device_t)(MDC_SLOT1+slot), (uint8)retval);
	
	return retval;
}

static int usb_write(uint16 data)
{
	 return bsp_spi_operate(SSI0_BASE, data);
}

static int usb_getc(mdc_slot_t slot, uint8 *rx)
{
	uint8 rd;
	
	if(Is_ring_buffer_empty((ringbuf_device_t)(MDC_SLOT1+slot)) == true){
		MDC_SPI_CS(0);
		rd = usb_write(0x00);
		usb_spi_shortDelay(40);
		if(rd == 0){
			MDC_SPI_CS(1);
			return -1;
		}else{
			*rx = rd;
		}
		
		MDC_SPI_CS(1);
	}else{
		rd = ring_buffer_read((ringbuf_device_t)(MDC_SLOT1+slot));
		*rx = rd;
	}
	
	return 0;
}


static int usb_bl_getMsg(uint8 *msg, uint8 len)
{
	uint32 i = 0;
	
	bsp_delay_ms(1);
	
	while(usb_write(USB_BL_MSG_END) != USB_BL_MSG_START){
	   bsp_delay_ms(1);
     i++;
     if(i > USB_READ_TIMEOUT){
			 SYS_TRACE("\r\nWait USB_BL_MSG_START \r\n");
			 return -1;
		 }
	}
	
	for(i = 0; i < len; i++){
		msg[i] = usb_write(USB_BL_MSG_END);
	}
	
	i = usb_write(USB_BL_MSG_END);
	if(i != USB_BL_MSG_END){
		SYS_TRACE("USB_BL_GetMsg error, %d\r\n", i);
		return -1;
	}
	
	return 0;
}

void mdc_usb_send_msg(mdc_slot_t slot, char *fmt, ...)
{
	uint8 j;
	size_t size;
	char msg[128] = {0};
  va_list ap;
  
	va_start(ap, fmt);
  vsprintf(msg, fmt, ap);
  va_end(ap);

  size = strlen(msg);

	mdc_bus_select(slot);
	
	usb_enable_spi();
	usb_read(slot, USB_MSG_END);
	usb_read(slot, USB_MSG_START);
	usb_read(slot, USB_MSG_RS232);
	usb_read(slot, size);
	for(j = 0; j < size; j++){
		usb_read(slot, msg[j]);
	}
	usb_read(slot ,USB_MSG_END);
	usb_disable_spi();
	
	mdc_bus_select(SLOT_NUM);
}


int usb_bl_check_flash_status(mdc_slot_t slot)
{
	uint8 status;
	int ret = 0;
	
	mdc_bus_select(slot);
	
  usb_enable_spi();
	usb_write(USB_BL_MSG_END);
	usb_write(USB_BL_MSG_START);
	usb_write(USB_BL_MSG_FLASH_STATE);
	
	ret = usb_bl_getMsg(&status, 1);
	usb_disable_spi();
	
	if(ret < 0) return ret;
	
	if(status == USB_BL_MSG_FLASH_OK){
		SYS_TRACE("get MSG_FLASH_OK \r\n");
		ret = 0;
	}else if(status == USB_BL_MSG_FLASH_BAD){
		SYS_TRACE("get MSG_FLASH_BAD \r\n");
		ret = -1;
	}else{
		SYS_TRACE("usb_bl_check_flash_status get %d\r\n", status);
		ret = -1;
	}
	
	mdc_bus_select(SLOT_NUM);
	
	return ret;
}

int usb_bl_get_version(mdc_slot_t slot)
{
	uint8 version[4] = {0};
  int ret;

  mdc_bus_select(slot);

  usb_enable_spi();
	usb_write(USB_BL_MSG_END);
  usb_write(USB_BL_MSG_START);
  usb_write(USB_BL_MSG_GET_VERSION);
  ret = usb_bl_getMsg(version, 4);
  usb_disable_spi();

  if(ret < 0){
		SYS_TRACE("usb_bl_get_version fail\r\n");
	}else{
		gMdc->slot[slot].ver.major = version[1];
		gMdc->slot[slot].ver.minor = version[2];
		gMdc->slot[slot].ver.beta = version[3];
		SYS_TRACE("usb_bl_get_version:%x.%x.%x\r\n", version[1], version[2], version[3]);
	}

	 mdc_bus_select(SLOT_NUM);

	return ret;
}

int usb_bl_runApp(mdc_slot_t slot, uint8 src_id)
{
	uint8 i;
	uint32 ret;
	uint32 rep;
	uint32 cur;
	uint8 len;
	char temp[25] = {0};
	
	mdc_bus_select(slot);
	
	usb_enable_spi();
	usb_write(USB_BL_MSG_END);
	usb_write(USB_BL_MSG_START);
	usb_write(USB_BL_MSG_RUN_APP);
	usb_disable_spi();
	
	rep = 1;
	cur = gTickCount;
	do{
		bsp_delay_ms(5);
		usb_enable_spi();
		ret = usb_write(USB_MSG_END);
		if(ret == USB_MSG_START){
			for(i = 0; i < 10; i++){
				ret = usb_write(USB_MSG_END);
				if(ret == USB_MSG_QUERY){
					ret = usb_write(USB_MSG_END);
					if(ret == USB_MSG_END){
						rep = 0;
						usb_disable_spi();
						break;
					}
				}
			}
		}
		usb_disable_spi();
		
		if(bsp_get_elapsedMs(cur) > 2000){
			mdc_bus_select(SLOT_NUM);
			SYS_TRACE("usb_bl_runApp timeout\r\n");
			return -1;
		}
		
	}while(rep);
	
	usb_enable_spi();
	usb_write(USB_MSG_END);
	usb_write(USB_MSG_START);
	usb_write(USB_MSG_QUERY);
	usb_write(BRDG_HOST_M12);
	usb_write(USB_MSG_END);
	usb_disable_spi();
	
	SYS_TRACE("send slot id %d to mdc usb card.\r\n", src_id);
	
	sprintf(temp, "Media.SetSourceID=%d\n", src_id);
	len = strlen(temp);
	
	SYS_TRACE("usb_bl_runApp->%s\r\n", temp);
	
	usb_enable_spi();
	usb_write(USB_MSG_END);
	usb_write(USB_MSG_START);
	usb_write(USB_MSG_RS232);
	usb_write(len);
	for(i = 0; i < len; i++){
		usb_write(temp[i]);
	}
	usb_write(USB_MSG_END);
	usb_disable_spi();
	
	
	mdc_bus_select(SLOT_NUM);
	
	return 0;
}

int mdc_usb_init(mdc_slot_t slot, uint8 src_id)
{
	int ret = 0;
	uint8 j;
	
	for(j = 0; j < SLOT_NUM; j++){
		CommandBuf[j][0] = 0;
	}
	
	mdc_bus_select(slot);
	
	if(usb_bl_check_flash_status(slot) < 0){
		SYS_TRACE("slot[%d],USB not start up\r\n", slot);
		ret = -1;
	}else{
		ret = usb_bl_get_version(slot);
		if(ret < 0){
			SYS_TRACE("USB: get version error %d\r\n", ret);
		}
		
		ret = usb_bl_runApp(slot, src_id);
	}
	
	mdc_bus_select(SLOT_NUM);
	
	return ret;
}

void usb_select_source(mdc_slot_t slot, uint8 src_id)
{
	char buf[16] = {0};
  uint8 len;
  uint8 j;

  sprintf(buf, "Main.Source=%d\n", src_id);
  len = strlen(buf);
  SYS_TRACE("slot[%d],usb_select_source->%s\r\n",slot,  buf);

  mdc_bus_select(slot);

  usb_enable_spi();
	usb_write(USB_MSG_END);
	usb_write(USB_MSG_START);
	usb_write(USB_MSG_RS232);
  usb_write(len);
  for(j = 0; j < len; j++){
		usb_write(buf[j]);
	}
	usb_write(USB_MSG_END);
  usb_disable_spi();
	
	mdc_bus_select(SLOT_NUM);
}

void mdc_usb_input_select(mdc_slot_t slot, usb_input_t in, uint8 src_id)
{
	switch(in){
		
		case USB_INPUT_PC:
			SYS_TRACE("Input select USB PC\r\n");
		  usb_select_source(slot, src_id + USB_INPUT_PC);
			break;
		
		case USB_INPUT_BACK:
			SYS_TRACE("Input select USB BACK\r\n");
		  mdc_usb_bus_select(slot);
		  usb_select_source(slot, src_id + USB_INPUT_BACK);
			break;
		
		case USB_INPUT_FRONT:
			SYS_TRACE("Input select USB FRONT\r\n");
		  mdc_usb_bus_select(slot);
		  USB_SWITCH_TO_MDC;
		  usb_select_source(slot, src_id + USB_INPUT_FRONT);
			break;
		
		default:
			break;
	}
	
}


void mdc_usb_task(mdc_slot_t slot)
{
	uint8 byte = 0;
	uint8 i;
	char msg[MSG_MAX_LEN] = {0};
  uint32 t;

  //t = bsp_get_curMs() % 3600000;

  //SYS_TRACE("mdc_usb_task: %02d:%02d:%03d\r\n", (t / 1000) / 60,  (t / 1000) % 60, t % 1000);
	
	mdc_bus_select(slot);
	
	mdc_pa_set(USB_PA);
	
	usb_getc(slot, &byte);
	
	if(byte == USB_MSG_START){
		CommandBuf[slot][0] = 1;
    CommandIndex[slot] = 1;
	}
	
	while(CommandBuf[slot][0]){
		if(byte == USB_MSG_START){
			
		}else if(byte == USB_MSG_END){
			if(CommandIndex[slot] > 1){
				
				if(CommandBuf[slot][1] == USB_MSG_RS232){
					memcpy(&msg[3],&CommandBuf[slot][3], CommandBuf[slot][2]);
					SYS_TRACE("slot[%d]: %s\r\n",slot, &msg[3]);
					
						if(gMdc->mdc_slot_src_index == USB_INPUT_BACK
							|| gMdc->mdc_slot_src_index == USB_INPUT_FRONT){
								sys_mdc_usb_msg_handler(slot, &msg[3]);
							}
					
				}else{
					
					SYS_TRACE("msg -> MAIN : ");
				  for(i = 0; i < CommandIndex[slot]; i++){
						if(CommandBuf[slot][i] > 0x1f && CommandBuf[slot][i] < 0x7f){
							SYS_TRACE("%c", CommandBuf[slot][i]);
						}else{
							SYS_TRACE("'%2x'", CommandBuf[slot][i]);
						}
					}
						SYS_TRACE("\r\n");	
				}
			}
			
			CommandBuf[slot][0] = 0;
			CommandIndex[slot] = 0;
			break;
		}else if(CommandBuf[slot][0]){
			CommandBuf[slot][CommandIndex[slot]] = byte;
			CommandIndex[slot]++;
			if(CommandIndex[slot] > MSG_MAX_LEN){
				SYS_TRACE("cmd overflow!%s\r\n", CommandBuf[slot]);
				CommandBuf[slot][0] = 0;
				CommandIndex[slot] = 0;
			}
		}
		byte = 0;
		usb_getc(slot, &byte);
	}
	
	mdc_bus_select(SLOT_NUM);
	
}





void mdc_usb_task2(mdc_slot_t slot)
{
	uint8 byte = 0;
	uint8 i;
	char msg[MSG_MAX_LEN] = {0};
  static uint8 state = USB_WAIT_START;
	
	mdc_bus_select(slot);
	
	mdc_pa_set(USB_PA);
	
	usb_getc(slot, &byte);

  switch(state){
		
		case USB_WAIT_START:
			if(byte == USB_MSG_START){
				state = USB_WAIT_END;
			}
			break;
			
		case USB_WAIT_END:
			break;
		
		
	}
  

	
	if(byte == USB_MSG_START){
		CommandBuf[slot][0] = 1;
    CommandIndex[slot] = 1;
	}
	
	while(CommandBuf[slot][0]){
		if(byte == USB_MSG_START){
			
		}else if(byte == USB_MSG_END){
			if(CommandIndex[slot] > 1){
//				SYS_TRACE("msg -> MAIN : ");
//				for(i = 0; i < CommandIndex[slot]; i++){
//					if(CommandBuf[slot][i] > 0x1f && CommandBuf[slot][i] < 0x7f){
//						SYS_TRACE("%c", CommandBuf[slot][i]);
//					}else{
//						SYS_TRACE("'%2x'", CommandBuf[slot][i]);
//					}
//				}
//				SYS_TRACE("\r\n");
				
				if(CommandBuf[slot][1] == USB_MSG_RS232){
					memcpy(&msg[3],&CommandBuf[slot][3], CommandBuf[slot][2]);
					SYS_TRACE("slot[%d]: %s\r\n",slot, &msg[3]);
					
//						if(gMdc->mdc_slot_src_index == USB_INPUT_BACK
//							|| gMdc->mdc_slot_src_index == USB_INPUT_FRONT){
								sys_mdc_usb_msg_handler(slot, &msg[3]);
							//}
					
				}
			}
			
			CommandBuf[slot][0] = 0;
			CommandIndex[slot] = 0;
			break;
		}else if(CommandBuf[slot][0]){
			CommandBuf[slot][CommandIndex[slot]] = byte;
			CommandIndex[slot]++;
			if(CommandIndex[slot] > MSG_MAX_LEN){
				SYS_TRACE("cmd overflow!%s\r\n", CommandBuf[slot]);
				CommandBuf[slot][0] = 0;
				CommandIndex[slot] = 0;
			}
		}
		byte = 0;
		usb_getc(slot, &byte);
	}
	
	mdc_bus_select(SLOT_NUM);
	
}


int mdc_usb_erase_flash(mdc_slot_t slot)
{
	int retval = 0;
	uint8 ready = 0;
	
	mdc_bus_select(slot);
	
	usb_enable_spi();
	usb_write(USB_MSG_END);
	usb_write(USB_MSG_START);
	usb_write(USB_BL_MSG_FLASH_ERASE);
	
	while(1){
		
		bsp_delay_ms(500);
		
		usb_write(USB_MSG_END);
	  usb_write(USB_MSG_START);
	  usb_write(USB_BL_MSG_FLASH_CHECK_READY);
		
		retval = usb_bl_getMsg(&ready, 1);
		
		if(retval == 0 && ready == USB_BL_MSG_FLASH_READY){
			break;
		}else if(ready == USB_BL_MSG_FLASH_BUSY){
			SYS_TRACE("mdc usb: Erase flash - busy\r\n");
		}else{
			SYS_TRACE("USB_BL_MSG_FLASH_CHECK_READY: NO ACK\r\n");
		}
	}
	
	usb_disable_spi();
	mdc_bus_select(SLOT_NUM);
	
	SYS_TRACE("Mdc Usb Flash Erase finished\r\n");
	
	return 0;
}


void mdc_usb_finish_program(mdc_slot_t slot, uint16 crc)
{
	mdc_bus_select(slot);
	
	usb_enable_spi();
	usb_write(USB_MSG_END);
	usb_write(USB_MSG_START);
	usb_write(USB_BL_MSG_FINISH);
	usb_write(crc >> 8);
	usb_write(crc & 0xff);
	
	usb_disable_spi();
	mdc_bus_select(SLOT_NUM);
	
	SYS_TRACE("Mdc Usb Flash CRC=0x%04x\r\n", crc);
	
}

static uint16 update_crc(uint16 crc, uint8 data)
{
	int j;
	
	crc ^= data;
	
	for(j = 0; j < 8; j++){
		if(crc & 1){
			crc >>= 1;
			crc ^= 0xa001;
		}else{
			crc >>= 1;
		}
	}
	
	return crc;
}



int mdc_usb_send_block(mdc_slot_t slot, DWORD_VAL addr, uint8 *block, uint16 len, uint16 *crc)
{
	uint16 j;
	int retval = 0;
	uint8 next = 0;
	
	mdc_bus_select(slot);
	
	usb_enable_spi();
	usb_write(USB_MSG_END);
	usb_write(USB_MSG_START);
	usb_write(USB_BL_MSG_BLOCK);
	
	usb_write(addr.byte.MB);
	usb_write(addr.byte.UB);
	usb_write(addr.byte.HB);
	usb_write(addr.byte.LB);
	
	for(j = 0; j < len; j++){
		usb_write(block[j]);
		*crc = update_crc(*crc, block[j]);
	}
	
	for(j = len; j < 512; j++){
		usb_write(0xff);
		*crc = update_crc(*crc, 0xff);
	}
	
	retval = usb_bl_getMsg(&next, 1);
	
	usb_disable_spi();
	mdc_bus_select(SLOT_NUM);
	
	if(retval < 0 || next != USB_BL_MSG_NEXT_BLOCK){
		SYS_TRACE("mdc_usb_send_block:error[%d]-[%d]\r\n", retval, next);
		return -1;
	}
	
	
	return 0;
}










