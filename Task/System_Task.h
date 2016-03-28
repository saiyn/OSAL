#ifndef _SYSTEM_TASK_H_
#define _SYSTEM_TASK_H_




extern sys_state_t *gSystem_t;

extern uint8 gSysTaskID;

extern menu_id_t gCurMenu;

extern uint8 ir_channel[4];

extern src_list_t *gSrcListHead;
extern src_list_t *gCurSrc;

extern file_list_t *gFileListHead[SLOT_NUM];
extern file_list_t *gCurFile[SLOT_NUM];

extern uint8 gcurrent_usb_file_cnt;

extern char *gCurTitle[SLOT_NUM];
extern uint16 gPlayTime;

extern char *gSrollBuf;

typedef struct
{
	    osal_event_hdr_t hdr;     /* OSAL Message header, must be in the first*/
	    uint8 value;
}Sys_Msg_t;


void SysTaskInit(uint8 task_id);

void sys_src_list_append(src_list_t *, mdc_setup_t *, mdc_slot_state_t *slot);

uint16 SysEventLoop(uint8 task_id, uint16 events);

void sys_src_load_cur(sys_state_t *sys, src_list_t **cur);

void Send_SysTask_Msg(sys_event_t event, uint8 value);

void sys_src_list_create(sys_state_t *sys, src_list_t **head);

void sys_src_list_detach(src_list_t **head);

void sys_input_select(sys_state_t *sys);

void sys_master_vol_set(sys_state_t *sys);

void sys_database_init(void);

void sys_mdc_usb_msg_handler(mdc_slot_t slot, char *msg);

void sys_mdc_blu_msg_handler(mdc_slot_t slot, char *msg);

void sys_cur_src_state_update(src_state_t state, uint8);

void sys_audio_valid_handler(uint8 index);

void sys_audio_invalid_handler(uint8 index);

void sys_trigger_plug_out_handler(uint8 index);

void sys_trigger_plug_in_handler(uint8 index);

void sys_trigger_valid_handler(uint8 index);

void sys_trigger_invalid_handler(uint8 index);

void sys_digital_audio_valid_handler(uint8 index);

void sys_digital_audio_invalid_handler(uint8 index);

void sys_hp_plug_in_handler(uint8 index);

void sys_hp_plug_out_handler(uint8 index);

void sys_nad_uart_msg_handler(sys_state_t *sys, char *msg);

void sys_update_all_src_state(src_state_t state);

void sys_update_src_state(src_state_t state, uint8 src_id);

void sys_auto_standby_poll(sys_state_t *sys);

void sys_output_mode_select(sys_state_t *sys);

void sys_protection_valid_handler(uint8 index);

void sys_protection_invalid_handler(uint8 index);

src_state_t sys_get_src_state(char *name);

void sys_line_out_mode_select(sys_state_t *sys);

void sys_update_src_state_by_name(src_state_t state, char *name);

void fix_pop_begin(void);

void fix_pop_end(void);

void do_main_msg_handler(sys_state_t *sys);

#endif


