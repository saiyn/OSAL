#ifndef _MDC_BT_H_
#define _MDC_BT_H_



typedef enum
{
    BT_INPUT=0,
    BT_INPUT_N
}bt_input_t;

typedef enum
{
    BOS_MSG_END = 0,
    BOS_MSG_START, 
    BOS_MSG_HOST,
    BOS_MSG_RS232,
    BOS_MSG_UP,
    BOS_MSG_DOWN,
    BOS_MSG_LEFT,
    BOS_MSG_RIGHT,
    BOS_MSG_NEXT,
    BOS_MSG_PREV,
    BOS_MSG_FFW,
    BOS_MSG_FRW,
    BOS_MSG_PLAY,
    BOS_MSG_PAUSE,
    BOS_MSG_ENTER,
    BOS_MSG_SHUFFLE,
    BOS_MSG_REPEAT,
    BOS_MSG_DISPLAY,
    BOS_MSG_GOTO_MENU,
    BOS_MSG_VOLUME,
    BOS_MSG_MUTE    
}bos_msg_t;

enum {
    BLU_BRDG_HOST_UNSET=0,
    BLU_BRDG_HOST_C390,
    BLU_BRDG_HOST_M12,
};

typedef enum{
    BLU_STATE_WORKING_NORMAL = 0,
    BLU_STATE_FACTROY_RESET,
    BLU_STATE_UPGRADE,
    BLU_STATE_SERVICE,
    BLU_STATE_NULL,
}blu_state_t;

typedef struct{
	uint8 slot;
	uint8 src_index;
	bool en;
	blu_state_t state;
	bool lockState;
	uint32 lockTmr;
	uint8 lockProgress;
	
}mdc_bluos_t;


typedef struct
{
	UINT8		volume_control[BT_INPUT_N];
	UINT8		volume_fixed[BT_INPUT_N];
	bool		enable[BT_INPUT_N];
	char 		name[BT_INPUT_N][MAX_INPUT_NAME_LEN];
    
}blu_setup_t;

extern char * const DEF_BLU_CMD[SLOT_NUM][BT_INPUT_N];

extern char * const DEF_BLU_NAME[2][BT_INPUT_N];


void blu_hw_init(mdc_slot_t slot, uint8 src_index);

void mdc_blu_task(mdc_slot_t slot);

void blu_send_msg(mdc_slot_t slot, char *fmt, ...);

#endif




