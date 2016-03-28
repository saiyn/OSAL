#ifndef _SOFT_TIMER_TASK_H_
#define _SOFT_TIMER_TASK_H_


extern uint8 gSTTID;

typedef struct
{
	    osal_event_hdr_t hdr;     /* OSAL Message header, must be in the first*/
	    uint8 value;
}Soft_Task_Msg_t;




void TimerOutEventInit(uint8 task_id);


uint16 TimerOutEventLoop(uint8 task_id, uint16 events);

void Send_Soft_Task_Msg(Soft_task_event_t event, uint8 value);


#endif






