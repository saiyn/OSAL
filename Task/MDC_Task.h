#ifndef _MDC_TASK_H_
#define _MDC_TASK_H_


extern uint8 gMdcTaskID;

typedef enum{
	MDC_VOLUME_MSG = 0,
	MDC_MUTE_MSG,
	MDC_MSG_NUM
}Mdc_event_t;

typedef struct
{
	    osal_event_hdr_t hdr;     /* OSAL Message header, must be in the first*/
	    uint8 value;
}Mdc_Task_Msg_t;

extern Mdc_Task_Msg_t *gMdcMsg_t;

void MdcTaskInit(uint8 task_id);


uint16 MdcEventLoop(uint8 task_id, uint16 events);

void Send_MdcTask_Msg(Mdc_event_t event, uint8 value);

#endif


