#ifndef _OSAL_TASK_K_
#define _OSAL_TASK_K_




#define RT_USING_UART2

#define USE_CONSOLE

#define TTS_MSG_ARRIVAL 0x01
#define DAB_MSG_ARRIVAL 0x02
#define KEY_MSG_ARRIVAL 0x03
#define DISPLAY_MSG_ARRIVAL 0x04

#define VOL_DEFAULT  10

#define BUZEER_DEFAULT_FREQENCY 200

#define DEFAULT_SOURSE DAB

#define SYS_MENU_TIMEOUT_THRESHOLD  1000*30UL




typedef enum
{
//	DAB_DISPALY_LOCK,
//	RTC_DISLPAY_LOCK,
	  DISPLAY_SCROLL_LOCK = 0,
  	DISPLAY_UPDATE_LOCK,
  	AUTO_STANDBY_TIMER_LOCK,
  	USB_FILE_LIST_SHOW_LOCK,
//	PALY_STATUS_LED_FLASH_LOCK,
//	UPDATE_DLS_BUFFER_LOCK,
//	DLS_SCROLL_LOCK,
//	OLED_SLEEP_FLAG,
	  FLAG_UNVALID = 31
}bitmapflag_t;


typedef enum
{
	 DAB =0,
	 FM,
	 BT,
	 AUX,
	 SOURSE_MAX
}sourse_t;

typedef struct
{
	 uint8 volume; 
	 uint32 fmfreq;
	 uint8 keybitmap;
	 //sys_state_t state;
	 sourse_t 	sourse;
	 uint16 bitmapflag;
	 bool isbtconnected;
}sys_infor_t;

extern sys_infor_t gSys_Infor;

extern bool IntMasterDisable(void);

extern bool IntMasterEnable(void);

#define TASK_NO_TASK      0xFF
#define MALLOC  malloc
#define FREE    free
#define DISABLE_INTERRUPT  IntMasterDisable
#define ENABLE_INTERRUPT   IntMasterEnable


void Delay_ms(uint32 time);

void BoardInit(void);

/*
*    Event handler function prototype
*/
typedef uint16 (*pTaskEventHandler)(uint8 task_id, uint16 event);

extern void osalInitTasks( void );
extern const pTaskEventHandler gTaskArr[];
extern const uint8 gTaskCnt;
extern  uint16 *pTaskEvents;
extern uint8 osal_init_system( void );
extern void osal_start_system( void );

void running_param_init(void);

void Register_Bitmap_Flag(bitmapflag_t flag);

void Deregister_Bitmap_Flag(bitmapflag_t flag);

bool IS_Flag_Valid(bitmapflag_t flag);

#endif


