#ifndef _OSAL_H_
#define _OSAL_H_


/*** Generic Status Return Values ***/
#define SUCCESS                   0x01
#define FAILURE                   0x00
#define INVALIDPARAMETER          0x02
#define INVALID_TASK              0x03
#define MSG_BUFFER_NOT_AVAIL      0x04
#define INVALID_MSG_POINTER       0x05
#define INVALID_EVENT_ID          0x06
#define NO_TIMER_AVAIL            0x07


#define SERIAL_POLL_MSG               (1 << 0)
#define MDC_POLL_MSG                  (1 << 1)
#define SYS_POWER_POLL_MSG            (1 << 2)   
#define BT_POLL_MSG                   (1 << 3)
#define LCD_SCROLL_MSG                (1 << 4)
#define NAD_UART_POLL_MSG             (1 << 5)
#define DATABASE_SAVE_MSG             (1 << 6)
#define VOL_DIS_BACK_MSG              (1 << 7)
#define SRC_MENU_TIMEOUT_MSG          (1 << 8)
#define SYS_CONFIG_MENU_BACK_MSG      (1 << 9)
#define SEND_BLUOS_RE_MSG             (1 << 10)
#define AUTO_STANDBY_MSG       				(1 << 11)
#define AUTO_STANDBY_POLL_MSG           (1 << 12)
#define USB_FILE_LIST_UPDATE_MSG          (1 << 13)
#define SRC_MENU_TO_FILE_LIST_TIMEOUT_MSG  (1 << 14)


#define SYS_USB_READING_TIMEOUT_MSG               (1 << 0)
#define SYS_SRC_UPDATE_TIMEOUT_MSG                (1 << 1)
#define SYS_POWER_ON_LATER_MSG                   (1 << 3)

/*MDC Task*/
#define HDMI_V2_VOLUME_PUSH_MSG               (1 << 2)


/*NAD APP TASK*/
#define PROTECTION_LATER_MSG                  (1 << 1)
#define FIX_SOURCE_CHANGE_POP_MSG             (1 << 2)
#define FIX_LINE_OUT_MODE_CHANGE_POP_MSG      (1 << 3)
#define FIX_SPEAKER_OUT_MODE_CHANGE_POP_MSG   (1 << 4)

/*soft timer task*/
#define IR_IN_POLARITY_CHECK_MSG             (1 << 13)
#define MDC_TEST_POLL_MSG                    (1 << 14)

/*upgrade task*/
#define UPGRADE_POLL_SMG                     (1 << 0)

//#define DOWN_KEY_LONG_DOWN_MSG      (1 << 0) //thought it is the same as UART_POLL_MSG, but they handled in different task 

#define SYS_EVENT_MSG               0x8000

typedef struct 
{
	   void *next;
	   uint16 len;
	   uint16 dest_id;
}osal_message_hdr_t;

typedef struct 
{
	  uint8 event;
	  uint8 status;
}osal_event_hdr_t;

typedef void *osal_msg_q_t;

#define OSAL_MSG_NEXT(msg_ptr)   ((osal_message_hdr_t *)(msg_ptr) - 1)-> next

#define OSAL_MSG_ID(msg_ptr)  ((osal_message_hdr_t *)(msg_ptr) - 1)-> dest_id

extern osal_msg_q_t osal_qHead;

extern uint8 *osal_msg_allocate(uint16 len);
extern uint8 osal_msg_send( uint8 destination_task, uint8 *msg_ptr );
extern uint8 osal_msg_deallocate( uint8 *msg_ptr );
extern uint8 *osal_msg_receive( uint8 task_id );

uint8 osal_set_event( uint8 task_id, uint16 event_flag );
#endif



