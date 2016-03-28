#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Timer.h"
#include "Soft_Timer_Task.h"
#include "serial.h"
#include "Mdc.h"
#include "System_Task.h"
#include "Display_Task.h"

uint8 gSTTID;

Soft_Task_Msg_t *gSoft;

void TimerOutEventInit(uint8 task_id)
{
	 gSTTID = task_id;
}


static void lcd_scroll_handler(sys_state_t *sys)
{
	if(sys->status == STATUS_STANDBY) return;
	
	if(IS_Flag_Valid(DISPLAY_SCROLL_LOCK) == true) return;
	
	dis_print_scroll(sys->scroll_line);
	
	osal_start_timerEx(gSTTID, LCD_SCROLL_MSG,  1000);
}


static void ir_in_polarity_check_handler(void)
{
	if(IR_IN_LEVEL){
		GPIOIntTypeSet(IR_IN_PORT, IR_IN_PIN, GPIO_FALLING_EDGE);
	}else{
		GPIOIntTypeSet(IR_IN_PORT, IR_IN_PIN, GPIO_RISING_EDGE);
	}
	
	
	osal_start_timerEx(gSTTID, IR_IN_POLARITY_CHECK_MSG, 150);
}

uint16 TimerOutEventLoop(uint8 task_id, uint16 events)
{
	Soft_Task_Msg_t *msg = NULL;
	
	if(events & SYS_EVENT_MSG)
	{
		msg = (Soft_Task_Msg_t *)osal_msg_receive(task_id);
			 
		while(msg)
		{
			switch(msg->hdr.event)
			{
				case SOFT_USER_MSG:
					mdc_test_task_handler(gSystem_t, true, msg->value);
					break;
								
				default:
				break;
			}
					 
			osal_msg_deallocate((uint8 *) msg);
					 
		  msg = (Soft_Task_Msg_t *)osal_msg_receive(task_id);
	  }
				
	  return (events ^ SYS_EVENT_MSG);
 }
	
	if(events & LCD_SCROLL_MSG){
		lcd_scroll_handler(gSystem_t);
		return(events ^ LCD_SCROLL_MSG);
	}
	
	if(events & AUTO_STANDBY_POLL_MSG){
		sys_auto_standby_poll(gSystem_t);
		return(events ^ AUTO_STANDBY_POLL_MSG);
	}
	
	if(events & SERIAL_POLL_MSG){
		serial_poll_handler();
		return(events ^ SERIAL_POLL_MSG);
	}
	
	if(events & MDC_TEST_POLL_MSG){
		mdc_test_task_handler(gSystem_t, false, 0);
		return(events ^ MDC_TEST_POLL_MSG);
	}
	
	if(events & IR_IN_POLARITY_CHECK_MSG){
		
		ir_in_polarity_check_handler();
		return(events ^ IR_IN_POLARITY_CHECK_MSG);
	}
	
	
	
	return 0;
}

void Send_Soft_Task_Msg(Soft_task_event_t event, uint8 value)
{
	   uint8 result = 0;
		 
	   gSoft = (Soft_Task_Msg_t *)osal_msg_allocate(sizeof(Soft_Task_Msg_t));
		 if(NULL == gSoft)
     {
			 SYS_TRACE("no memory for soft task msg\r\n");
			 return;
		 }
		 gSoft->hdr.event = (uint8)event;	
		 gSoft->value = value;
	 
		 result = osal_msg_send(gSTTID,(uint8 *)gSoft);
		 if(result != SUCCESS)
		 {
			  SYS_TRACE("send soft task msg fail\r\n");
		 }
}


