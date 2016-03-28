#ifndef _DISPLAY_TASK_H_
#define _DISPLAY_TASK_H_

extern uint8 gDisTaskID;

typedef enum{
	DIS_UPDATE=0,
	DIS_JUMP,
}dis_event_t;

typedef enum{
	ALIGN_LEFT = 0,
	ALIGN_CENTER,
	ALIGN_RIGHT
}dis_align_t;


typedef struct
{
	    osal_event_hdr_t hdr;     /* OSAL Message header, must be in the first*/
	    uint8 value;
}Dis_Msg_t;

extern menu_t *gMenuHead;

extern menu_t *cur_menu;


void DisplayTaskInit(uint8 task_id);

void display_menu_list_create(menu_t **head);

uint16 DisplayTaskEventLoop(uint8 task_id, uint16 events);

void display_menu_jump(menu_id_t to);

void Send_DisplayTask_Msg(dis_event_t event, uint8 value);

void dis_print_scroll(uint8 line);

void dis_scroll_update(char *buf);

void display_init_menu(void);

void display_menu_state_update(menu_id_t id, menu_state_t state);


void display_factory_menu(void);

void dis_simple_print(uint8 line, char *fmt, ...);

#endif







