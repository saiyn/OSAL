#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Mdc.h"
#include "Mdc_bt.h"
#include "OSAL_RingBuf.h"
#include "System_Task.h"

static mdc_bluos_t   mdc_bluos;

char * const DEF_BLU_CMD[SLOT_NUM][BT_INPUT_N] = {
    {"BluOS"},
    {"BluOS"}
};


char * const DEF_BLU_NAME[2][BT_INPUT_N] = {
    {"BluOS"},
    {"BluOS "},
};

#define BLU_HW_WAITING_TMR_FACTORY_RESET   35000
#define BLU_HW_WAITING_TMR_UPGRADE         10000

#define BLU_HW_WAITING_TMR_SERVICE         5000

#define BLU_HW_WAITING_TMR_POWER_CUT       2000
#define BLU_HW_WAITING_TMR_POWER_RESUME    500

static uint8 CommandIndex[SLOT_NUM] = {0};
static uint8 CommandBuf[SLOT_NUM][MSG_MAX_LEN];

static uint8 sloop=0;

static void blu_spi_shortDelay(uint8 a)
{
    sloop = a;
    do{
        sloop--;
    }while(sloop > 0);
}

static int blu_read(mdc_slot_t slot, uint16 data)
{
	int retval;
	
	OSAL_ASSERT(slot < SLOT_NUM);
	
	retval = bsp_spi_operate(SSI0_BASE, data);
	
	ring_buffer_write((ringbuf_device_t)(MDC_SLOT1+slot), (uint8)retval);
	
	
	return retval;
}

static int blu_write(uint16 data)
{
	 return bsp_spi_operate(SSI0_BASE, data);
}


static int blu_getc(mdc_slot_t slot, uint8 *rx)
{
	uint8 rd;
	
	if(Is_ring_buffer_empty((ringbuf_device_t)(MDC_SLOT1+slot)) == true){
		MDC_SPI_CS(0);
		rd = blu_write(0x00);
		blu_spi_shortDelay(20);
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

static void blu_hw_set(mdc_slot_t slot)
{
	 if(slot == SLOT_1){
		 GPIO_PIN_SET(GPIO2M0_PORT, GPIO2M0_PIN, 1);
		 GPIO_PIN_SET(GPIO3M0_PORT, GPIO3M0_PIN, 0);
	 }else if(slot == SLOT_2){
		 GPIO_PIN_SET(GPIO2M1_PORT, GPIO2M1_PIN, 1);
		 GPIO_PIN_SET(GPIO3M1_PORT, GPIO3M1_PIN, 0);
	 }
}

static int blu_resume_power(void)
{
	if(gMdc->slot[mdc_bluos.slot].type == MDC_BLUOS){
		SYS_TRACE("Resume power\r\n");
		mdc_power_set((mdc_slot_t)mdc_bluos.slot, 1);
		return 0;
	}
	
	return -1;
}

static void blu_hw_normal(mdc_slot_t slot)
{
	 switch(slot){
		 case SLOT_1:
			 GPIO_PIN_SET(GPIO2M0_PORT, GPIO2M0_PIN, 0);
		   GPIO_PIN_SET(GPIO3M0_PORT, GPIO3M0_PIN, 1);
			 break;
		 
		 case SLOT_2:
			 GPIO_PIN_SET(GPIO2M1_PORT, GPIO2M1_PIN, 0);
		   GPIO_PIN_SET(GPIO3M1_PORT, GPIO3M1_PIN, 1);
			 break;
		 
		 default:
			 break;
	 }
}

static void blu_hw_factory(mdc_slot_t slot)
{
	 switch(slot){
		 case SLOT_1:
			 GPIO_PIN_SET(GPIO2M0_PORT, GPIO2M0_PIN, 0);
		   GPIO_PIN_SET(GPIO3M0_PORT, GPIO3M0_PIN, 0);
			 break;
		 
		 case SLOT_2:
			 GPIO_PIN_SET(GPIO2M1_PORT, GPIO2M1_PIN, 0);
		   GPIO_PIN_SET(GPIO3M1_PORT, GPIO3M1_PIN, 0);
			 break;
		 
		 default:
			 break;
	 }
}


void blu_hw_init(mdc_slot_t slot, uint8 src_index)
{
	uint8 j;
	
	mdc_bus_select(slot);
	
	blu_hw_normal(slot);
	
	mdc_bus_select(SLOT_NUM);
	
	for(j = 0; j < SLOT_NUM; j++){
		CommandBuf[j][0] = 0;
	}
	
	mdc_bluos.slot = slot;
	mdc_bluos.src_index = src_index;
	mdc_bluos.en = true;
	mdc_bluos.state = BLU_STATE_WORKING_NORMAL;
	mdc_bluos.lockState = false;
	mdc_bluos.lockProgress = 0;
}

void blu_send_msg(mdc_slot_t slot, char *fmt, ...)
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
	
	MDC_SPI_CS(0);
	blu_read(slot, BOS_MSG_END);
	blu_read(slot, BOS_MSG_START);
	blu_read(slot, BOS_MSG_RS232);
	blu_read(slot, size);
	for(j = 0; j < size; j++){
		blu_read(slot, msg[j]);
	}
	blu_read(slot, BOS_MSG_END);
	MDC_SPI_CS(1);
}

static void blu_hw_service(mdc_slot_t slot)
{
	if(mdc_bluos.lockState == false) return;
	
	if(mdc_bluos.state == BLU_STATE_SERVICE){
		if(mdc_bluos.lockProgress == 0){
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_POWER_CUT){
				SYS_TRACE("enter service mode\r\n");
				blu_hw_set(slot);
				mdc_bluos.lockTmr = bsp_get_curMs();
				mdc_bluos.lockProgress++;
			}
		}else if(mdc_bluos.lockProgress == 1){
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_POWER_RESUME){
				blu_resume_power();
				mdc_bluos.lockTmr = bsp_get_curMs();
				mdc_bluos.lockProgress++;
			}
		}else{
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_SERVICE){
				SYS_TRACE("backup normal mode\r\n");
				blu_hw_normal(slot);
				mdc_bluos.state = BLU_STATE_WORKING_NORMAL;
				mdc_bluos.lockState = false;
			}
		}
	}
	if(mdc_bluos.state == BLU_STATE_FACTROY_RESET){
		if(mdc_bluos.lockProgress == 0){
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_POWER_CUT){
				SYS_TRACE("enter factory reset mode\r\n");
				blu_hw_factory(slot);
				mdc_bluos.lockTmr = bsp_get_curMs();
				mdc_bluos.lockProgress++;
			}
		}else if(mdc_bluos.lockProgress == 1){
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_POWER_RESUME){
				blu_resume_power();
				mdc_bluos.lockTmr = bsp_get_curMs();
				mdc_bluos.lockProgress++;
			}
		}else{
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_FACTORY_RESET){
				SYS_TRACE("backup normal mode\r\n");
				blu_hw_normal(slot);
				mdc_bluos.state = BLU_STATE_WORKING_NORMAL;
				mdc_bluos.lockState = false;			
			}
		}
	}
	if(mdc_bluos.state == BLU_STATE_UPGRADE){
		if(mdc_bluos.lockProgress == 0){
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_POWER_CUT){
				SYS_TRACE("enter upgrade mode\r\n");
				blu_hw_factory(slot);
				mdc_bluos.lockTmr = bsp_get_curMs();
				mdc_bluos.lockProgress++;
			}
		}else if(mdc_bluos.lockProgress == 1){
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_POWER_RESUME){
				blu_resume_power();
				mdc_bluos.lockTmr = bsp_get_curMs();
				mdc_bluos.lockProgress++;
			}
		}else{
			if(bsp_get_elapsedMs(mdc_bluos.lockTmr) > BLU_HW_WAITING_TMR_UPGRADE){
				SYS_TRACE("backup normal mode\r\n");
				blu_hw_normal(slot);
				mdc_bluos.state = BLU_STATE_WORKING_NORMAL;
				mdc_bluos.lockState = false;
			}
		}
	}
	
	
}

void blu_report_hid(mdc_slot_t slot, uint8 src_id)
{
	uint8 i;
	uint8 len;
	char temp[64] = {0};
	
	
	mdc_bus_select(slot);
	
	MDC_SPI_CS(0);
	blu_read(slot, BOS_MSG_END);
	blu_read(slot, BOS_MSG_START);
	blu_read(slot, BOS_MSG_HOST);
	blu_read(slot, BLU_BRDG_HOST_M12);
	blu_read(slot, BOS_MSG_END);
	MDC_SPI_CS(1);
	
  sprintf(temp, "Media.SetSourceID=%d\n", src_id);
  len = strlen(temp);

  MDC_SPI_CS(0);
	blu_read(slot, BOS_MSG_END);
	blu_read(slot, BOS_MSG_START);
	blu_read(slot, BOS_MSG_RS232);
	blu_read(slot, len);
	for(i = 0; i < len; i++){
		blu_read(slot, temp[i]);
	}
	blu_read(slot, BOS_MSG_END);
	MDC_SPI_CS(1);

	mdc_bus_select(SLOT_NUM);
}



void mdc_blu_task(mdc_slot_t slot)
{
	uint8 byte;
	uint8 i;
	char msg[MSG_MAX_LEN] = {0};
  uint32 t;

 // t = bsp_get_curMs() % 3600000;

  //SYS_TRACE("mdc_blu_task: %02d:%02d:%03d\r\n", (t / 1000) / 60,  (t / 1000) % 60, t % 1000);

  mdc_bus_select(slot);

	blu_hw_service(slot);

  blu_getc(slot, &byte);

  if(byte == BOS_MSG_START){
	  CommandBuf[slot][0] = 1;
    CommandIndex[slot] = 1;
	}
	
	while(CommandBuf[slot][0]){
		if(byte == BOS_MSG_START){
			
		}else if(byte == BOS_MSG_END){
			if(CommandIndex[slot] > 1){
				
				if(CommandBuf[slot][1] == BOS_MSG_RS232){
					memcpy(msg, &CommandBuf[slot][3], CommandBuf[slot][2]);
					SYS_TRACE("slot[%d] : %s\r\n", slot, msg);
					sys_mdc_blu_msg_handler(slot, msg);
				}else if(CommandBuf[slot][1] == BOS_MSG_HOST && CommandIndex[slot] == 2){
					SYS_TRACE("BOS_MSG_HOST:%d\r\n", gMdc->slot_src_idx[slot]);
					//blu_report_hid(slot, mdc_bluos.src_index);
					blu_report_hid(slot, gMdc->slot_src_idx[slot]);
				}
			}
			
			CommandBuf[slot][0] = 0;
      CommandIndex[slot] = 0;
			break;
		}else if(CommandBuf[slot][0]){
			CommandBuf[slot][CommandIndex[slot]] = byte;
			CommandIndex[slot]++;
			if(CommandIndex[slot] > MSG_MAX_LEN){
				SYS_TRACE("cmd overflow!\r\n");
				CommandBuf[slot][0] = 0;
				CommandIndex[slot] = 0;
			}
		}
		
		byte = 0;
		blu_getc(slot, &byte);
	}

  mdc_bus_select(SLOT_NUM);
}


















