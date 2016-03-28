#ifndef _OSAL_TIMER_H_
#define _OSAL_TIMER_H_

typedef struct
{
	  void *next;
	  int timeout;
	  uint16 event_flag;
	  uint8 task_id;
	  int reloadTimeout;
}oaslTimer_t;

void osal_stop_all_timerEx(void);

uint8 osal_start_timerEx( uint8 taskID, uint16 event_id, int timeout_value );

uint8 osal_start_reload_timer( uint8 taskID, uint16 event_id, int timeout_value );

uint8 osal_stop_timerEx( uint8 task_id, uint16 event_id );

void osalTimerUpdate(void);











#endif



