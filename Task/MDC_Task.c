#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Timer.h"
#include "MDC_Task.h"
#include "serial.h"
#include "Mdc.h"
#include "System_Task.h"


uint8 gMdcTaskID;

Mdc_Task_Msg_t *gMdcMsg_t;

void MdcTaskInit(uint8 task_id)
{
	 gMdcTaskID = task_id;
}

static void do_mdc_volume_report(sys_state_t *sys, mdc_slot_t slot)
{
	uint8 vol;
	
	vol = ((int)sys->master_vol - MASTER_VOL_INDEX_MIN_INT) * 100 / MASTER_VOL_WIDTH_INT; 
	
	SYS_TRACE("do_mdc_volume_report:vol=%d\r\n", vol);
	hdmi_v2_send_msg(slot, HDMI2_CMD_VOLUME, &vol, 1);
}

static void mdc_volume_event_handler(sys_state_t *sys)
{
	mdc_t *p;
	uint8 j;
	
	if(sys->status == STATUS_STANDBY) return;
	
	p = (mdc_t *)sys->mdc;
	
	for(j = 0; j < SLOT_NUM; j++){
		if(p->slot[j].type != MDC_CARD_NUM){
			SYS_TRACE("mdc_volume_event_handler:%d-%d-%d\r\n", p->slot_src_idx[j], sys->src, (p->slot_src_idx[j] + p->slot[j].max_input));
			if(sys->src >= p->slot_src_idx[j] && sys->src < (p->slot_src_idx[j] + p->slot[j].max_input)){
				switch(p->slot[j].type){
	
					case MDC_HDMI_v2:
						do_mdc_volume_report(sys, (mdc_slot_t)j);
						break;
					
					default:
						break;
				}
				
			}
		}
	
	}
	
}

static void mdc_poll_handler(sys_state_t *sys)
{
	mdc_t *p;
	uint8 j;
	
	if(sys->status == STATUS_STANDBY) return;
	
	p = (mdc_t *)sys->mdc;
	
	for(j = 0; j < SLOT_NUM; j++){
		if(p->slot[j].type != MDC_CARD_NUM){
			//if(sys->src >= p->slot_src_idx[j] && sys->src <= (p->slot_src_idx[j] + p->slot[j].max_input)){
				switch(p->slot[j].type){
	
					case MDC_USB:
						mdc_usb_task((mdc_slot_t)j);		
						break;
					
					case MDC_BLUOS:
						mdc_blu_task((mdc_slot_t)j);
						break;
					
					case MDC_HDMI_v2:
						hdmi_v2_task((mdc_slot_t)j);
						break;
					
					default:
						break;
				}
				
				
				osal_start_timerEx(gMdcTaskID, MDC_POLL_MSG,  10);
			//}
		}
	
	}
}

uint16 MdcEventLoop(uint8 task_id, uint16 events)
{
	Mdc_Task_Msg_t *msg = NULL;
	
	if(events & SYS_EVENT_MSG)
	{
		msg = (Mdc_Task_Msg_t *)osal_msg_receive(task_id);
			 
		while(msg)
		{
			switch(msg->hdr.event)
			{
				
				case MDC_VOLUME_MSG:
					mdc_volume_event_handler(gSystem_t);
					break;
				
				default:
				break;
			}
					 
			osal_msg_deallocate((uint8 *) msg);
					 
		  msg = (Mdc_Task_Msg_t *)osal_msg_receive(task_id);
	  }
				
			return (events ^ SYS_EVENT_MSG);
 }
	
	
	if(events & MDC_POLL_MSG){
		mdc_poll_handler(gSystem_t);
		return (events ^ MDC_POLL_MSG);
	}
	
	if(events & HDMI_V2_VOLUME_PUSH_MSG){
		
		Send_MdcTask_Msg(MDC_VOLUME_MSG, 0);
		
		return (events ^ HDMI_V2_VOLUME_PUSH_MSG);
	}
	
	return 0;
}

void Send_MdcTask_Msg(Mdc_event_t event, uint8 value)
{
	   uint8 result = 0;
		 
	   gMdcMsg_t = (Mdc_Task_Msg_t *)osal_msg_allocate(sizeof(Mdc_Task_Msg_t));
		 if(NULL == gMdcMsg_t)
     {
			 SYS_TRACE("no memory for mdc msg\r\n");
			 return;
		 }
		 gMdcMsg_t->hdr.event = (uint8)event;	
		 gMdcMsg_t->value = value;
	 
		 result = osal_msg_send(gMdcTaskID,(uint8 *)gMdcMsg_t);
		 if(result != SUCCESS)
		 {
			  SYS_TRACE("send mdc msg fail\r\n");
		 }
}


