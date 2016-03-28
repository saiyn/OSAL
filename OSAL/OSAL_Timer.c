/*
* Copyright (c) 2014, Hansong (Nanjing) Technology Ltd.
* All rights reserved.
* 
* ----File Info ----------------------------------------------- -----------------------------------------
* Name:     OSAL_Timer.c
* Author:   saiyn.chen
* Date:     2014/4/20
* Summary:  this is the os abstract kernel file.


* Change Logs:
* Date           Author         Notes
* 2014-04-20     saiyn.chen     first implementation
* 2014-07-08     saiyn.chen     Add osal_stop_all_timerEx
*/
#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Timer.h"


oaslTimer_t *gtimerHead;

oaslTimer_t *osalAddTimer(uint8 task_id, uint16 event_flag, int timeout);
oaslTimer_t *osalFindTimer(uint8 task_id, uint16 event_flag);
void osalDeleteTimer(oaslTimer_t *timer);


/*********************************************************************
 * API FUNCTIONS
*/

/*************************************************************************
*  @fn      osalAddTimer
*  
*  @brief   This function add a timer to the timer list.
*           
*  @param   task_id - which task will be scheduled.
*           event_flag - what event to set 
*           timeout - you know what
*
*  @return   pointer to newly created timer
*/

oaslTimer_t *osalAddTimer(uint8 task_id, uint16 event_flag, int timeout)
{
	   oaslTimer_t *newTimer;
	   oaslTimer_t *srchTimer;
	
	   /*look for an existing timer first*/
	   newTimer = osalFindTimer(task_id, event_flag);
	   if(newTimer)
		 {
			   /*Timer is found then just update it*/
			   newTimer->timeout = timeout;
			   
			   return ( newTimer );
		 }
		 else
		 {
			    newTimer = MALLOC(sizeof(oaslTimer_t));
			 
			    if(newTimer)
					{
						   newTimer->task_id = task_id;
						   newTimer->event_flag = event_flag;
						   newTimer->timeout = timeout;
						   newTimer->next = NULL;
						   newTimer->reloadTimeout = 0;
					
					
					/*Does the timer list already extist*/
					if(gtimerHead == NULL)
					{
						   gtimerHead = newTimer;
					}
					else
					{
						   /*add it to the end of the timer list*/
						
						  srchTimer = gtimerHead;
						
						  while(srchTimer->next)
							{
								   srchTimer = srchTimer->next;
							}
							
							srchTimer->next = newTimer;
					}
					
					return (newTimer);
		 }
		 else
		   return ((oaslTimer_t *)NULL);
	 }
		 
}


/*************************************************************************
*  @fn      osalFindTimer
*  
*  @brief   This function try to find a timer in the timer list.
*           
*  @param   task_id - which task will be scheduled.
*           event_flag - what event to set 
*
*  @return   found timer or the end timer
*/

oaslTimer_t *osalFindTimer(uint8 task_id, uint16 event_flag)
{
	    oaslTimer_t *srchTimer;
	
	    srchTimer = gtimerHead;
	
	    while(srchTimer)
			{
				   if(srchTimer->event_flag == event_flag &&
						 srchTimer->task_id == task_id)
					  break;
					  
				srchTimer = srchTimer->next;
			}
			
		return (srchTimer);
}

/*************************************************************************
*  @fn      osalDeleteTimer
*  
*  @brief   This function delete a timer from a timer list.
*           
*  @param   timer - which timer will be deleted.
*           
*
*  @return  none
*/

void osalDeleteTimer(oaslTimer_t *timer)
{
	   if(timer)
		 {
			    timer->event_flag = 0;
		 }
}

/*************************************************************************
*  @fn      osal_start_timerEx
*  
*  @brief   This function is called to start a timer to rxpire in mSecs.
*           When the timer expires, the calling task will get the specified event.
*           
*  @param   taskID - which task will get the event.
*           event_id - which event will be set.
*           timeout_value - in milliseconds
*
*  @return  SUCCESS, or NO_TIMER_AVAIL.
*/
uint8 osal_start_timerEx( uint8 taskID, uint16 event_id, int timeout_value )
{
     uint32 temp;
	   oaslTimer_t *newTimer;
	
	   /*hold off all interrupts*/
     temp = DISABLE_INTERRUPT();
	
	   newTimer = osalAddTimer(taskID, event_id, timeout_value);
	   
	   ENABLE_INTERRUPT();
	
	  (void)temp;
	
	  return ((newTimer != NULL) ? SUCCESS :NO_TIMER_AVAIL );
}

/*************************************************************************
*  @fn      osal_start_reload_timer
*  
*  @brief   This function is called to start a timer to expire in mSecs.
*           When the timer expires, the calling task will get the specified event,
*           and the timer will be reloaded.
*           
*  @param   taskID - which task will get the event.
*           event_id - which event will be set.
*           timeout_value - in milliseconds
*
*  @return  SUCCESS, or NO_TIMER_AVAIL.
*/
uint8 osal_start_reload_timer( uint8 taskID, uint16 event_id, int timeout_value )
{
     uint32 temp;
	   oaslTimer_t *newTimer;
	
	   /*hold off all interrupts*/
     temp = DISABLE_INTERRUPT();
	
	   newTimer = osalAddTimer(taskID, event_id, timeout_value);
	   if(newTimer)
		 {
			   newTimer->reloadTimeout = timeout_value;
		 }
	   
	   ENABLE_INTERRUPT();
		 
		 (void)temp;
	
	  return ((newTimer != NULL) ? SUCCESS :NO_TIMER_AVAIL );
}

/*************************************************************************
*  @fn      osal_stop_timerEx
*  
*  @brief   This function is called to stop a timer that has already been started.
*           
*           
*           
*  @param   taskID - which task will get the event.
*           event_id - which event will be set.
*          
*
*  @return  SUCCESS, or NO_TIMER_AVAIL.
*/
uint8 osal_stop_timerEx( uint8 task_id, uint16 event_id )
{
     uint32 temp;
	   oaslTimer_t *foundTimer;
	
	   /*hold off all interrupts*/
    temp = DISABLE_INTERRUPT();
	
	   foundTimer = osalFindTimer(task_id, event_id);
	   if(foundTimer)
		 {
			   osalDeleteTimer( foundTimer );
		 }
	   
	   ENABLE_INTERRUPT();
		 
		 (void)temp;
	
	  return ((foundTimer != NULL) ? SUCCESS :NO_TIMER_AVAIL );
}


/*************************************************************************
*  @fn      osal_stop_all_timerEx
*  
*  @brief   This function is called to stop all timer that has already been started.
*           
*           
*           
*  @param   none
*          
*
*  @return  none
*/

void osal_stop_all_timerEx(void)
{
	    oaslTimer_t *srchTimer;
	
	    srchTimer = gtimerHead;
	
	    while(srchTimer)
			{
				osalDeleteTimer(srchTimer);	  
				srchTimer = srchTimer->next;
			}  
}


/*************************************************************************
*  @fn      osalTimerUpdate
*  
*  @brief   This function is the main api and will be called in systick interrupt.
*           
*                     
*  @param   none
*           
*          
*  @return  none
*/
void osalTimerUpdate(void)
{
    uint32 temp;
	  oaslTimer_t *srchTimer;
	  oaslTimer_t *prevTimer;
	
	
	  if(gtimerHead != NULL)
		{
			   srchTimer = gtimerHead;
			   prevTimer = NULL;
			
			/*look throught the timer list*/
			while(srchTimer)
			{
				  oaslTimer_t *freeTimer = NULL;
				  
				   /*hold off all interrupts*/
          temp = DISABLE_INTERRUPT();
				
				   /*check and update every timer*/
				   if(srchTimer->timeout <= 0)
					 {
						   srchTimer->timeout = 0;
					 }
					 else
					 {
						    srchTimer->timeout--;
					 }
					 
					 /*check for reloading*/
					 if((srchTimer->timeout == 0) && (srchTimer->reloadTimeout) && (srchTimer->event_flag))
					 {
						    /*Notify the task of a timeout*/
						   osal_set_event(srchTimer->task_id, srchTimer->event_flag);
						   /*reload the timeout*/
						   srchTimer->timeout = srchTimer->reloadTimeout;
					 }
					 
					 /*when timeout or deleted*/
					 if(srchTimer->timeout == 0 || srchTimer->event_flag == 0)
					 {
						    if(prevTimer == NULL)
								{
									  gtimerHead = srchTimer->next;
								}
								else
								{
									   prevTimer->next = srchTimer->next;								 
								}
								
								freeTimer = srchTimer ;
								
								srchTimer = srchTimer->next;
					 }
					 else
					 {
						    prevTimer = srchTimer;
						    srchTimer = srchTimer->next;
					 }
				  
					 ENABLE_INTERRUPT();
					 
					 if(freeTimer)
					 {
						    if(freeTimer->timeout == 0)
								{
									  osal_set_event(freeTimer->task_id, freeTimer->event_flag);
								}
								
								FREE(freeTimer);
					 }
			}
		}
		
		(void)temp;

}










