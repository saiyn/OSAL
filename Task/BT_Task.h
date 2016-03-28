#ifndef _BT_TASK_H_
#define _BT_TASK_H_


extern uint8 gBT_TaskID;




typedef struct
{
	    osal_event_hdr_t hdr;     /* OSAL Message header, must be in the first*/
	    uint8 value;
}Bt_Task_Msg_t;

void bt_send_byte(uint8 byte);

uint16 BtTaskEventLoop(uint8 task_id, uint16 events);

void BtTaskEventInit(uint8 task_id);

void bt_clear_all_paring_list(void);


#endif

