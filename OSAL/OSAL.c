/*
* Copyright (c) 2014, Hansong (Nanjing) Technology Ltd.
* All rights reserved.
* 
* ----File Info ----------------------------------------------- -----------------------------------------
* Name:     OSAL.c
* Author:   saiyn.chen
* Date:     2014/4/20
* Summary:  this is the os abstract kernel file.


* Change Logs:
* Date           Author         Notes
* 2014-04-20     saiyn.chen     first implementation
*/

#include "sys_head.h"
#include "OSAL_Task.h"
#include "OSAL.h"

#define TASK_TRACE(...)  //rt_kprintf



/*Message Pool Definitions*/

osal_msg_q_t osal_qHead;

/*********************************************************************
 * API FUNCTIONS
 *********************************************************************/
 
 
/*************************************************************************
*  @fn      osal_set_event
*  
*  @brief   This function is called to set the event flags for a task.
*           
*  @param   task_id - receiving task id     
*           event_flag - what event to set 
*
*  @return   SUCCESS, INVALID_TASK
*/
uint8 osal_set_event( uint8 task_id, uint16 event_flag )
{
	   uint32 temp;
	
	   if(task_id < gTaskCnt)
		 {
			    temp = DISABLE_INTERRUPT();
			    pTaskEvents[task_id] |= event_flag;
			    ENABLE_INTERRUPT();
			 
			    (void)temp;
			    return SUCCESS;
		 }
		 else
		 {
			  return INVALID_TASK;
		 }
		 
		 
}

/*************************************************************************
*  @fn      osal_clear_event
*  
*  @brief   This function is called to clear the event flags which have been handled.
*           
*  @param   task_id - receiving task id     
*           event_flag - what event to clear 
*
*  @return   SUCCESS, INVALID_TASK
*/
uint8 osal_clear_event( uint8 task_id, uint16 event_flag )
{
	   uint32 temp;
	
	   if(task_id < gTaskCnt)
		 {
			    temp = DISABLE_INTERRUPT();
			    pTaskEvents[task_id] &= ~event_flag;
			    ENABLE_INTERRUPT();
			   (void)temp;
			    return SUCCESS;
		 }
		 else
		 {
			  return INVALID_TASK;
		 }
		 
	
}

/*************************************************************************
*  @fn      osal_msg_enqueue
*  
*  @brief   This function enqueues an OSAL message into an OSAL queue.
*           
*  @param   q_ptr - OSAL queue      
*           msg_ptr - OSAL message
*
*  @return  none
*/
void osal_msg_enqueue( osal_msg_q_t *q_ptr, void *msg_ptr )
{
	   void *list;
	   uint32 temp;
	
	   /*hold off all interrupts*/
     temp = DISABLE_INTERRUPT();
	   OSAL_MSG_NEXT( msg_ptr ) = NULL;
	   /*if first message inqeue*/
	   if(NULL == *q_ptr)
		 {
			    *q_ptr = msg_ptr;
		 }
		 else
		 {
			   /*find end of queue*/
			   for(list = *q_ptr; OSAL_MSG_NEXT(list) != NULL; list =OSAL_MSG_NEXT(list));
			 
			   /*add message to end of the queue*/
			   OSAL_MSG_NEXT(list) = msg_ptr;
		 }
		 
		 ENABLE_INTERRUPT();
		 
		 (void)temp;
}

/*************************************************************************
*  @fn      osal_msg_dequeue
*  
*  @brief   This function dequeues an OSAL message from an OSAL queue.
*           
*  @param   q_ptr - OSAL queue      
*          
*
*  @return  pointer to OSAL message or NULL if queue is empty
*/
void *osal_msg_dequeue( osal_msg_q_t *q_ptr )
{
	
	   void *msg_ptr = NULL;
	   uint32 temp;
	
	   /*hold off all interrupts*/
     temp = DISABLE_INTERRUPT();
	
	   if(*q_ptr != NULL)
		 {
			   msg_ptr = *q_ptr;
			   *q_ptr =  OSAL_MSG_NEXT(msg_ptr);
			   OSAL_MSG_NEXT( msg_ptr ) = NULL;
         OSAL_MSG_ID( msg_ptr ) = TASK_NO_TASK;
		 }
	  
	   ENABLE_INTERRUPT();
		 
		 (void)temp;
		 
		 return msg_ptr;
}
 
/*************************************************************************
*  @fn      osal_msg_allocate
*  
*  @brief   This function is called by a task to allocate a message buffer 
*           into which the task will encode the particular message it wish to send.
*         
*  @param   len - wanted buffer length
*
*  @return  pointer to allocated buffer or NULL if allocation failed.
*/

uint8 *osal_msg_allocate(uint16 len)
{
	  osal_message_hdr_t *hdr;
	
	  hdr = (osal_message_hdr_t *)MALLOC(sizeof(osal_message_hdr_t) + len);
	  if(NULL == hdr) return NULL;
	
	  hdr->next = NULL;
	  hdr->len = len;
	  hdr->dest_id = TASK_NO_TASK;
	  
	  TASK_TRACE("osal msg allocate success\r\n");
	  return ((uint8 *)(hdr + 1));
}

/*************************************************************************
*  @fn      osal_msg_deallocate
*  
*  @brief   This function is called  to deallocate a message buffer, which means
*           a task has finished processing a received meaasge.
*         
*  @param   msg_ptr - pointer to message buffer
*
*  @return  SUCCESS, INVALID_MSG_POINTER
*/

uint8 osal_msg_deallocate( uint8 *msg_ptr )
{
	  uint8 *ptr = NULL;
	
	  if(NULL == msg_ptr) return INVALID_MSG_POINTER;
	
	  /*don't deallocate a queued buffer*/
	  if ( OSAL_MSG_ID( msg_ptr ) != TASK_NO_TASK )
    return ( MSG_BUFFER_NOT_AVAIL );
		
		ptr = (uint8 *)((uint8 *)msg_ptr - sizeof(osal_message_hdr_t));
		
		FREE((void *)ptr);
		
		return SUCCESS;
}

/*************************************************************************
*  @fn      osal_msg_extract
*  
*  @brief   This function extract and remove a message from 
*           the middle of an osal queue.
*         
*  @param   q_ptr - osal queue
*           msg_ptr - which to be removed
*           prev_ptr - message befor msg_ptr in queue
*
*  @return  none
*/
void osal_msg_extract( osal_msg_q_t *q_ptr, void *msg_ptr, void *prev_ptr )
{
	   uint32 temp;
	
	   /*hold off all interrupts*/
     temp = DISABLE_INTERRUPT();
	
	   
	   if(msg_ptr == *q_ptr)
		 {
			   /*remove from first*/
			   *q_ptr = OSAL_MSG_NEXT( msg_ptr );
		 }
		 else
		 {
			   /*remove from middle*/
			   OSAL_MSG_NEXT(prev_ptr) = OSAL_MSG_NEXT(msg_ptr);
		 }
		 OSAL_MSG_NEXT( msg_ptr ) = NULL;
     OSAL_MSG_ID( msg_ptr ) = TASK_NO_TASK;
		 
		 ENABLE_INTERRUPT();
		 
		 (void)temp;
}


/*************************************************************************
*  @fn      osal_msg_send
*  
*  @brief   This function is called by a task to send a command
*           message to another task or processing element.
*         
*  @param   destination_task - send to which task
*           msg_ptr - pointer to message buffer
*
*  @return  SUCCESS, INVALID_TASK, INVALID_MSG_POINTER
*/
uint8 osal_msg_send( uint8 destination_task, uint8 *msg_ptr )
{
	  if ( msg_ptr == NULL )
    return ( INVALID_MSG_POINTER );
		
		if ( destination_task >= gTaskCnt )
    {
        osal_msg_deallocate( msg_ptr );
        return ( INVALID_TASK );
    }
		
		/*check the message head*/
		if ( OSAL_MSG_NEXT( msg_ptr ) != NULL ||
       OSAL_MSG_ID( msg_ptr ) != TASK_NO_TASK )
    {
        osal_msg_deallocate( msg_ptr );
        return ( INVALID_MSG_POINTER );
    }
		
		OSAL_MSG_ID( msg_ptr ) = destination_task;
		
		/*queue message*/
		osal_msg_enqueue(&osal_qHead, msg_ptr);
		
		/*signal the task that a message is watting*/
		osal_set_event(destination_task, SYS_EVENT_MSG);
		
		return SUCCESS;
}

/*************************************************************************
*  @fn      osal_msg_receive
*  
*  @brief   This function is called by a task to retrieve
*           a received message.
*         
*  @param   task_id - message belongs to which task 
*          
*
*  @return  message information or NULL if no message
*/
uint8 *osal_msg_receive( uint8 task_id )
{
	   osal_message_hdr_t *listHdr;
     osal_message_hdr_t *prevHdr = NULL;
     osal_message_hdr_t *foundHdr = NULL;
	   uint32 temp;
	
	   temp = DISABLE_INTERRUPT();
	
	   /*point to the top of the queue*/
	  listHdr = osal_qHead;
	 
	  /*look throught the queue for a message that belongs to the asking task */
	  while(listHdr != NULL)
		{
			   if((listHdr - 1)->dest_id == task_id)
				 {
					  if(foundHdr == NULL)
						{
					    foundHdr = listHdr;
						}
						else
						{
					    break;
						}
				 } 
				 if(foundHdr == NULL)
				 {
					   prevHdr = listHdr;
				 }
				 
				 listHdr = OSAL_MSG_NEXT(listHdr);
		}
		
		/*Is there more than one message*/
		if(listHdr != NULL)
		{
			  /*yes, signal the task that another message is waitting*/
			  osal_set_event( task_id, SYS_EVENT_MSG );
		}
		else
		{
			  /*no , current task has no more message to handle so let the os check next task */
			  osal_clear_event( task_id, SYS_EVENT_MSG );
		}
		
		/*  is there a message should be handled*/
		if(foundHdr != NULL)
		{
			   /*yes, take out of the link list*/
			   osal_msg_extract( &osal_qHead, foundHdr, prevHdr );			 
		}
		
		ENABLE_INTERRUPT();
		
		(void)temp;
		
		return((uint8 *)foundHdr);
}




















