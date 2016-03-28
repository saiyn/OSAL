#ifndef _SYS_DATABASE_H_
#define _SYS_DATABASE_H_


#define SUB_OUTPUT  0
#define BYPASS      1
#define HIGHPASS_OUTPUT 2

#define DIR_UP  1
#define DIR_DOWN 0
#define DIR_EQU  2


typedef enum{
	SYS_KEYBOARD_MEG=0,
	SYS_POWER_MSG,
	SYS_DATABASE_SAVE_MSG,
	SYS_VOL_UP,
	SYS_VOL_DOWN,
	SYS_VOL_SET,
	SYS_IR_MSG,
	SYS_MSG_NUM
}sys_event_t;

typedef enum{
    HDMI2_CMD_NULL=0,
    HDMI2_CMD_RUN_APP,
    HDMI2_CMD_GET_VERSION,
    HDMI2_CMD_INPUT_SEL,

    HDMI2_CMD_VOLUME = 0x0A,
    HDMI2_CMD_MUTE,
    
}HDMI2_cmd_t;

typedef enum{
	BT_LIMBO=0,
	BT_CONNECTABLE,
	BT_CONNDISCOVERABLE,
	BT_CONNECTED,
	
	BT_A2DPSTREAMING=0x0d,
}bt_status_t;

typedef enum{
	SOFT_MDC_MSG = 0,
	SOFT_USER_MSG,
	SOFT_MSG_NUM
}Soft_task_event_t;

typedef enum{
	BLU_INITING=0,
	CONNECTING_TO_NETWORK,
	HOTSPOT_MODE,
	MDC_BLU_STATUS_NUM
}mdc_blu_status_t;

typedef struct{
	uint8 datawith;
	uint8 protocal;
	uint32 bitrate;
}spi_ctl_t;


typedef enum{
	HC595_NULL = 0,
	HC595_HP_MUTE,
	HC595_LINE_MUTE,
	HC595_PS_EN,
	HC595_PS1,
	HC595_PS2,
	HC595_PS4,
	HC595_PS5
}hc595_io_t;

typedef enum{
	KEY_UP = 0,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_STANDBY,
	KEY_SRC_NEXT,
	KEY_SRC_PREV,
	KEY_ENTER,
	
	KEY_ENTER_LONG,
	KEY_SRC_PREV_LONG,
	KEY_SRC_NEXT_LONG,
	KEY_STANDBY_LONG,
	KEY_RIGHT_LONG,
	KEY_LEFT_LONG,
	KEY_DOWN_LONG,
	KEY_UP_LONG,
	KEY_VALUE_MAX
}key_value_t;

typedef enum{
	LED_OFF=0,
	LED_ORANGE,
	LED_RED,
	LED_BIUE
}led_state_t;

typedef enum{
	WELCOM_MENU = 0,
	SOURCE_MENU,
	USB_FILE_LIST_MENU,
	USB_PLAY_MENU,
	USB_NOT_INSERT_MENU,
	USB_READING_MENU,
	BLU_STATUS_MENU,
	BT_STATUS_MENU,
	AC_ERROR_MENU,
	OVER_TEMP_MENU,
	DC_ERROR_MENU,
	
	OUTPUT_SELECT_MENU, //loop begain
	LINE_OUT_SELECT_MENU,
	IR_CHANNEL_SELECT_MENU,
	AUTO_STANDBY_MENU,
	SPEAKE_MODE_SELECT_MENU,
	TREBEL_CONTROL_MENU,
	BASS_CONTROL_MENU,
	BALANCE_CONTROL_MENU,
  FIRMWARE_VERSION_MENU,
	FIRMWARE_UPGRADE_MENU,
  MENU_NUM
}menu_id_t;


typedef enum{
	SPEAKER_OUT_NEED_UPDATE = 0x80,
	LINE_OUT_NEED_UPDATE,
}dis_para_t;

typedef enum{
IR_VOL_UP =0,
IR_VOL_DOWN,
IR_ON,
IR_OFF,
IR_SOURCE_UP,
IR_SOURCE_DOWN,
IR_MUTE,
IR_PAUSE,
IR_PREV,
IR_PLAY,
IR_RTN,
IR_UP,
IR_LFET,
IR_CENTER,
IR_RIGHT,
IR_DOWN,
IR_REPEAT,
IR_SHUFFLE,
IR_NEXT,
IR_NUM1,
IR_NUM2,
IR_NUM3,
IR_NUM4,
IR_NUM5,
IR_NUM6,
IR_NUM7,
IR_NUM8,
IR_NUM9,
IR_NUM10,
IR_DIMMER,
IR_EQ,
IR_TONE,
IR_MENU,
IR_COMMOND_NUM
}ir_commond_t;

#define MENU_NODE  IP_ADDRESS_MENU


typedef enum{
	DETECT_TRIGGER_IN =0,
	DETECT_TRIGGER_LEVEL,
	DETECT_HP_INSERT,
	DETECT_AC_DET,
	DETECT_DC_ERROR,
#ifdef NAD_C378
	DETECT_DC2_ERROR,
#endif
	
	DETECT_OVER_TEMP_R,
#ifdef NAD_C378
	DETECT_OVER_TEMP_L,
#endif
	
	DETECT_AUDIO,
	DETECT_DIGITAL_AUDIO,
	NUM_OF_DETECT
}detect_event_t;


extern bool log_on_nad;

extern const char *version;

typedef enum{
	MENU_NORMAL=0,
	MENU_SCROLL,
}menu_state_t;

struct menu{
	menu_id_t id;
	menu_state_t state;
	struct menu *next;
	struct menu *prev;
	struct menu *child;
	struct menu *parent;
};

typedef struct menu menu_t;

typedef enum{
	SRC_STATE_IDEL=0,
	SRC_STATE_PLAY,
	SRC_STATE_PAUSE,
	SRC_STATE_FILE_LIST,
	SRC_STATE_NO_DEVICE,
}src_state_t;


struct src_list{
	uint8 id;
	src_state_t state;
	const char *src_name;
	struct src_list *next;
	struct src_list *prev;
};

typedef struct src_list src_list_t;


struct file_list{
	uint8 id;
	char *file_name;
	struct file_list *next;
	struct file_list *prev;
};

typedef struct file_list file_list_t;


typedef enum{
	MANUAL=0,
	AUDIO,
	TRIGGER,
	IR_IP,
	POWER_ON_MAX
}power_on_t;


#define	IR_CHANNEL_0  0xff
#define	IR_CHANNEL_1  0x0f
#define	IR_CHANNEL_2  0x17
#define	IR_CHANNEL_3  0x1b


typedef enum{
	STATUS_STANDBY=0,
	STAUS_INITING,
	STATUS_WORKING,
	STATUS_SHUTDOWN,
	STATUS_MAX
}sys_status_t;

typedef enum{
	SRC_COAX1=0,
	SRC_COAX2,
	SRC_OPT1,
	SRC_OPT2,
	SRC_MM,
	SRC_BT,
	SRC_LINE,
	SRC_CONSTANT_NUM
}src_t;

typedef enum{
	SENSE_3MV=0,
	SENSE_6MV,
	SENSE_9MV,
	SENSE_12MV,
	SENSE_15MV,
	SENSE_THRESHOLD_NUM
}sense_threshold_t;

typedef enum{
	SE_TIMEOUT_5=0,
	SE_TIMEOUT_10,
	SE_TIMEOUT_20,
	SE_TIMEOUT_30,
	SE_TIMEOUT_60,
	SE_TIMEOUT_NUM
}sense_timeout_t;

typedef enum{
	SPEAKER_A_B = 0,
	SPEAKER_A,
	SPEAKER_B,
	SPEAKER_OFF,
	SPEAKER_MODE_NUM
}speaker_mode_t;

typedef struct{
 
	uint8 src_num;
	src_t src;
	float master_vol;
	uint8 speak_output_mode;
	uint8 line_output_mode;
	uint8 ir_channel; 
	bool  is_auto_standby;
	speaker_mode_t speaker_mode;
	int treble_vol;
	int bass_vol;
	int balance_l;
	int balance_r;
	
	uint32 sys_crc32;
	uint32 mdc_crc32;
	uint8 end_of_nv;
	
	void *mdc;
	void *mdc_test;
	sys_status_t status;
	uint32 bitmapflag;
	uint8 protect_bitmap;
  uint8 scroll_line;
	char *sroll_buf;
	mdc_blu_status_t mbs[2];
	bool is_mute;
	bt_status_t bt_s;
	uint8 previous_src;
  

}sys_state_t;




#define SIZEOF_SYS_NV   ((uint8 *)&gSystem_t->end_of_nv - (uint8 *)&gSystem_t->src_num)


#endif


