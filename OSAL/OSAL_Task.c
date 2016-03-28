/*
* Copyright (c) 2014, Hansong (Nanjing) Technology Ltd.
* All rights reserved.
* 
* ----File Info ----------------------------------------------- -----------------------------------------
* Name:     OSAL_Task.c
* Author:   saiyn.chen
* Date:     2014/4/20
* Summary:  this file is where you can easily add your own tasks.


* Change Logs:
* Date           Author         Notes
* 2014-04-20     saiyn.chen     first implementation
*/
#include "sys_head.h"
#include "OSAL_Task.h"
#include "OSAL.h"
#include "OSAL_Timer.h"
#include "Soft_Timer_Task.h"
#include "OSAL_RingBuf.h"
#include "serial.h"
#include "Mdc.h"
#include "System_Task.h"
#include "BT_Task.h"
#include "Display_Task.h"
#include "74hc595.h"
#include "IR_driver.h"
#include "MDC_Task.h"
#include "NAD_Uart_App_Task.h"
#include "OSAL_Detect.h"


extern uint8 gUpgradeTaskID;


extern void UpgradeTaskInit(uint8 task_id);

extern uint16 UpgradeEventLoop(uint8 task_id, uint16 events);

/*
*   The order in this table must identical to the task initialization calls
*/
const pTaskEventHandler gTaskArr[] ={
	MdcEventLoop,
	DisplayTaskEventLoop,
	SysEventLoop,
	BtTaskEventLoop,
	NADUartTaskEventLoop,
	TimerOutEventLoop,
	UpgradeEventLoop
	
};

const uint8 gTaskCnt = sizeof( gTaskArr ) / sizeof( gTaskArr[0] );

uint16 *pTaskEvents;


/*********************************************************************
 * @fn      osalInitTasks
 *
 * @brief   This function invokes the initialization function for each task.
 *
 * @param   void
 *
 * @return  none
 */

void osalInitTasks( void )
{
	  unsigned char taskID = 0;
	
	  pTaskEvents = (unsigned short *)MALLOC(sizeof( unsigned short ) * gTaskCnt);
	  if(NULL == pTaskEvents) return;
	  memset(pTaskEvents, 0, (sizeof( unsigned short ) * gTaskCnt));
	
	  MdcTaskInit(taskID++);
	  DisplayTaskInit(taskID++);
	  SysTaskInit(taskID++);
	  BtTaskEventInit(taskID++);
	  NADUartTaskEventInit(taskID++);
    TimerOutEventInit(taskID++);
	  UpgradeTaskInit(taskID);
}



/*************************************************************************
*  @fn      osal_init_system
*  
*  @brief   This function initializes the "task" system and someting else.
*          
*         
*  @param   void
*          
*
*  @return  SUCCESS
*/

uint8 osal_init_system( void )
{
	/*Initialize the message queue*/
  osal_qHead = NULL;
	/*Initialize the system tasks*/
  osalInitTasks();
	
	sys_database_init();
	
	sys_src_list_create(gSystem_t, &gSrcListHead);
	
	ring_buffer_init();
	
	hc595_init();
	
	mdc_init();
	
	sys_src_load_cur(gSystem_t, &gCurSrc);
	
	display_menu_list_create(&gMenuHead);
	
	//	Enable UART0 interupt
	IntEnable(INT_UART0);
	
	//	Enable UART3 interupt
	IntEnable(INT_UART3);
	
	//	Enable UART6 interupt
	IntEnable(INT_UART6);

	return SUCCESS;
}

/*************************************************************************
*  @fn      osal_start_system
*  
*  @brief   This function is the main loop function of the task system.
*          
*         
*  @param   void
*          
*
*  @return  none
*/
void osal_start_system( void )
{
		   uint8 index = 0;
	     ir_commond_t cmd;
		
		   /*here can add some poll functions*/
	      if(ir_get_commond(IR_FRONT, &cmd)){
					
					Send_SysTask_Msg(SYS_IR_MSG, (uint8)cmd);
				}
				
				if(ir_get_commond(IR_BACK_IN, &cmd)){
					Send_SysTask_Msg(SYS_IR_MSG, (uint8)cmd);
				}
	
				DetectGpioPoll();
				
		   do{
				 
				   if(pTaskEvents[index]) break;
				    
			 }while(++index < gTaskCnt);
			 
			 /*schedule task which has received events to handle*/
			 if(index < gTaskCnt)
			 {
				   uint16 events;
				   uint32 temp;
				 
				   temp = DISABLE_INTERRUPT();
				   /*clear the events for the task*/
				   events = pTaskEvents[index];
				   pTaskEvents[index] = 0;
				   ENABLE_INTERRUPT();
				 
				   events = (gTaskArr[index])(index, events);
				 
				   temp = DISABLE_INTERRUPT();
				   /*add back unprocessed events the current task*/
				   pTaskEvents[index] |= events;
				   ENABLE_INTERRUPT();
				 
				   (void)temp;
			 }
}

void Delay_ms(uint32 time)
{
	   uint32 current;
	   
	   current = gTickCount;
	
	   while((gTickCount - current) < time)
		 {
		 }
}


void SysTickIntHandler(void)
{
	  osalTimerUpdate();
	  gTickCount++;
}

void Register_Bitmap_Flag(bitmapflag_t flag)
{
	  if(flag > FLAG_UNVALID) return;
	
	  gSystem_t->bitmapflag |= (1 << flag);
}

void Deregister_Bitmap_Flag(bitmapflag_t flag)
{
	   if(flag > FLAG_UNVALID) return;
	
	   gSystem_t->bitmapflag &= ~(1 << flag);
}

bool IS_Flag_Valid(bitmapflag_t flag)
{
	   if(flag > FLAG_UNVALID) return false;
	
	   return(((1 << flag) & gSystem_t->bitmapflag) ? true : false);
}


