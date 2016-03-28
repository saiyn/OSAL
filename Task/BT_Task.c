#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Timer.h"
#include "BT_Task.h"
#include "OSAL_RingBuf.h"
#include "Display_Task.h"
#include "Mdc.h"
#include "System_Task.h"
#include "Src4382.h"

#define BT_STATUS 0x02

uint8 gBT_TaskID;

Bt_Task_Msg_t *gBT;

void BtTaskEventInit(uint8 task_id)
{
	 gBT_TaskID = task_id;
}


static void do_bt_msg_handler(uint8 *msg, size_t len)
{
	uint8 j;
	
	switch(msg[0]){
		
		case BT_STATUS:
			//OSAL_ASSERT(len >= 4);
		  
		  gSystem_t->bt_s = msg[2];
			if(gSystem_t->bt_s == BT_A2DPSTREAMING){
				if(gSystem_t->src == SRC_BT){
						src4382_PortB_Mute_Control(OFF);
				}
				sys_update_src_state(SRC_STATE_PLAY, SRC_BT);
			}else{
				if(gSystem_t->src == SRC_BT){
						src4382_PortB_Mute_Control(ON);
				}
				sys_update_src_state(SRC_STATE_IDEL, SRC_BT);
			}
		
			if(gSystem_t->src != SRC_BT) return;
			
		  if(cur_menu->id == SOURCE_MENU){
				display_menu_jump(BT_STATUS_MENU);
				Send_DisplayTask_Msg(DIS_UPDATE, msg[2]);
			}else if(cur_menu->id == BT_STATUS_MENU){
				Send_DisplayTask_Msg(DIS_UPDATE, msg[2]);
			}
			
			
			break;
	}
	
}

void bt_poll_handler(void)
{
	   size_t len;
	   uint8 ch;
	   static uint8 buf[128] = {0};
     static  uint8 index = 0;
     uint8 peek = 0;
     uint8 j;

 
	
	   len = ring_buffer_len(BT_UART);

	   if(len > 0){
			  SYS_TRACE("get %d bytes data from BT:", len);
			  for(j = 0; j < len; j++){
					SYS_TRACE("%2x ", ch = ring_buffer_read(BT_UART));
					buf[index++] = ch;
					if(ch == 0x55){
						ring_buffer_peek_read(BT_UART, &peek, 1);
						if(peek == 0xff){
							ring_buffer_read(BT_UART);
							j++;
							do_bt_msg_handler(buf, index);
							memset(buf, 0, sizeof(buf));
							index = 0;
							continue;
						}
					}
					
					if(index > 127) index = 0;
				}
				SYS_TRACE("\r\n");
				
		 }else{
				//SYS_TRACE("bt_poll_handler, no data\r\n");
		 }			 
}

uint16 BtTaskEventLoop(uint8 task_id, uint16 events)
{
	Bt_Task_Msg_t *msg = NULL;
	
	if(events & SYS_EVENT_MSG)
	{
		msg = (Bt_Task_Msg_t *)osal_msg_receive(task_id);
			 
		while(msg)
		{
			switch(msg->hdr.event)
			{
								
				
				default:
				break;
			}
					 
			osal_msg_deallocate((uint8 *) msg);
					 
		  msg = (Bt_Task_Msg_t *)osal_msg_receive(task_id);
	  }
				
			return (events ^ SYS_EVENT_MSG);
 }
 
 if(BT_POLL_MSG & events){
	 
	 bt_poll_handler();
	 return (BT_POLL_MSG ^ events);
 }
	
	
	return 0;
}


void bt_send_byte(uint8 byte)
{
	while(UARTCharPutNonBlocking(UART6_BASE, byte) == false);
	//UARTCharPut(UART6_BASE, byte);
}

void bt_clear_all_paring_list(void)
{
	
	bt_send_byte(0x05);
	bt_send_byte(0x00);
	bt_send_byte(0x55);
	bt_send_byte(0xff);
	
}

















