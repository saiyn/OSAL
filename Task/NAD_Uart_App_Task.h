#ifndef _NAD_UART_APP_TASK_H_
#define _NAD_UART_APP_TASK_H_



extern uint8 gNADUartID;


typedef struct
{
	    osal_event_hdr_t hdr;     /* OSAL Message header, must be in the first*/
	    uint8 value;
}Nad_Uart_Task_Msg_t;



void NADUartTaskEventInit(uint8 task_id);



uint16 NADUartTaskEventLoop(uint8 task_id, uint16 events);


void nad_send_str(char *fmt, ...);






#endif

