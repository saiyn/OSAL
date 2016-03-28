#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Timer.h"
#include "NAD_Uart_App_Task.h"
#include "OSAL_RingBuf.h"
#include "Display_Task.h"
#include "Mdc.h"
#include "System_Task.h"
#include "OSAL_Detect.h"
#include "Soft_Timer_Task.h"
#include "74hc595.h"

bool nad_app_echo = false;

uint8 gNADUartID;

Nad_Uart_Task_Msg_t *gNADUart;

void NADUartTaskEventInit(uint8 task_id)
{
	 gNADUartID = task_id;
}

void nad_send_byte(uint8 byte)
{
	while(UARTCharPutNonBlocking(UART3_BASE, byte) == false);
}

void nad_send_str(char *fmt, ...)
{
	va_list ap;
	char buf[512] = {0};
  size_t len;
  size_t index = 0;

  va_start(ap, fmt);
	vsprintf(buf, fmt, ap);
  va_end(ap);
	
	len = strlen(buf);
  while(index < len){
		while(UARTCharPutNonBlocking(UART3_BASE, buf[index]) == false);
			index++;
		}
}


static void nad_uart_poll_handler(void)
{
	   size_t len = 0;
	   uint8 ch;
	   static char buf[128] = {0};
     static  uint8 index = 0;
	
	   len = ring_buffer_len(NAD_APP);

	   if(len > 0){
			 while(len--){
				    ch = ring_buffer_read(NAD_APP);
				    if(ch == '\b'){
							  nad_send_str("\b \b");
							  if(index > 0){
							    buf[--index] = 0;
								}
							  continue;
						}
						if(ch == '\r' || ch == '\n'){
							if(index == 0){
								if(nad_app_echo == true && gSystem_t->mdc_test == NULL){
								  nad_send_str("\r\nSaiyn->");
								}
								  continue;
							}
							else{
							  buf[index] = 0;
								if(gSystem_t->mdc_test == NULL){
									sys_nad_uart_msg_handler(gSystem_t, buf);
								}else{
									Send_Soft_Task_Msg(SOFT_USER_MSG, '\n');
								}
							  memset(buf, 0, sizeof(buf));
							  index = 0;
								continue;
							}
						}
						if(index > 127) index = 0;
						buf[index++] = ch;
						if(nad_app_echo == true && gSystem_t->mdc_test == NULL){
							nad_send_byte(ch);
						}else if(gSystem_t->mdc_test){
							Send_Soft_Task_Msg(SOFT_USER_MSG, ch);
						}
			 }
		 }		
	
}


static void protection_later_handler(sys_state_t *sys)
{
	detect_event_t event ;
	
	
	for(event = DETECT_AC_DET; event < DETECT_AUDIO; event++){
		if(sys->protect_bitmap & (1 << event)){
			update_detect_state(event);
			//break;
		}
	}

	 
}

uint16 NADUartTaskEventLoop(uint8 task_id, uint16 events)
{
	Nad_Uart_Task_Msg_t *msg = NULL;
	
	if(events & SYS_EVENT_MSG)
	{
		msg = (Nad_Uart_Task_Msg_t *)osal_msg_receive(task_id);
			 
		while(msg)
		{
			switch(msg->hdr.event)
			{
								
				
				default:
				break;
			}
					 
			osal_msg_deallocate((uint8 *) msg);
					 
		  msg = (Nad_Uart_Task_Msg_t *)osal_msg_receive(task_id);
	  }
				
			return (events ^ SYS_EVENT_MSG);
 }
 
 if(NAD_UART_POLL_MSG & events){
	 
	 nad_uart_poll_handler();
	 return (NAD_UART_POLL_MSG ^ events);
 }
 
 if(PROTECTION_LATER_MSG & events){
	 
	 protection_later_handler(gSystem_t);

	 return(PROTECTION_LATER_MSG ^ events);
 }
	
 if(FIX_SOURCE_CHANGE_POP_MSG & events){
	 
	 fix_pop_end();
	 
	 return(FIX_SOURCE_CHANGE_POP_MSG ^ events);
 }
 
 if(FIX_LINE_OUT_MODE_CHANGE_POP_MSG & events){
	 if(!IS_HP_INSERT_IN){
			LINE_MUTE_OFF();
	 }
	 
	 return (FIX_LINE_OUT_MODE_CHANGE_POP_MSG ^ events);
 }
 
 if(FIX_SPEAKER_OUT_MODE_CHANGE_POP_MSG & events){
	 
	 if(!IS_HP_INSERT_IN){
			AMP_MUTE_OFF();
	    AMP_MUTE2_OFF();
	 }
	 return(FIX_SPEAKER_OUT_MODE_CHANGE_POP_MSG ^ events);
 }
	
	return 0;
}

