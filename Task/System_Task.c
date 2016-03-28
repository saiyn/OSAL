#include "sys_head.h"
#include "OSAL_Task.h"
#include "OSAL.h"
#include "OSAL_Timer.h"
#include "Mdc.h"
#include "System_Task.h"
#include "OSAL_Soft_IIC.h"
#include "EPRom_driver.h"
#include "OSAL_Utility.h"
//#include "Mdc.h"
#include "Mdc_ana.h"
#include "Src4382.h"
#include "pcm1795.h"
#include "njw1194.h"
#include "Soft_Timer_Task.h"
#include "lcd_driver.h"
#include "UI.h"
#include "74hc595.h"
#include "OSAL_Console.h"
#include "BT_Task.h"
#include "Display_Task.h"
#include "MDC_Task.h"
#include "NAD_Uart_App_Task.h"
#include "OSAL_Detect.h"
#include "OSAL_Key.h"

#define FIX_SRC_CHANGE_POP_TIMEOUT  2000
#define FIX_LINE_OUT_MODE_CHANGE_POP_TIMEOUT  2000
#define FIX_SPEAKER_OUT_MODE_CHANGE_POP_TIMEOUT  2000

Sys_Msg_t *gSys;
uint8 gSysTaskID;
sys_state_t *gSystem_t;

menu_id_t gCurMenu;

src_list_t *gSrcListHead;
src_list_t *gCurSrc;

file_list_t *gFileListHead[SLOT_NUM];
file_list_t *gCurFile[SLOT_NUM];
uint16 gPlayTime;

char *gSrollBuf;

extern mdc_slot_t gSlot;

uint8 gcurrent_usb_file_cnt;
uint8 gCurrent_usb_record_file_cnt;
char *gCurTitle[SLOT_NUM];

static void power_key_msg_handler(sys_state_t *sys, key_value_t key);
static void sys_vol_control_handler(sys_state_t *sys, uint8 dir, uint8 step);
static void sys_usb_file_next(mdc_slot_t slot);
static void sys_usb_file_prev(mdc_slot_t slot);
static void do_delect_file_list(file_list_t **head, file_list_t **cur);
static void sys_show_config_menu_handler(sys_state_t *sys);
static void sys_bt_play_pause(void);
static void sys_treble_vol_set(sys_state_t *sys);
static void sys_bass_vol_set(sys_state_t *sys);

const char * const constant_src_name[SRC_CONSTANT_NUM]={
	"Digital Coax 1", "Digital Coax 2", "Optical 1", "Optical 2", "Phono MM", "Bluetooth", "Analog"
};

const char * const constant_src_name_2[SRC_CONSTANT_NUM]={
	"Digital Coax 1 M", "Digital Coax 2 M", "Optical 1 M", "Optical 2 M", "Phono MM", "Bluetooth", "Analog" 
};

uint8 ir_channel[4]={IR_CHANNEL_0, IR_CHANNEL_1, IR_CHANNEL_2, IR_CHANNEL_3};

void SysTaskInit(uint8 task_id)
{
	  gSysTaskID = task_id;
}

static void sys_database_save(sys_state_t *sys, uint8 type)
{
	 uint32 crc = 0;
	 int retval;
	
	 if(type == 1){
		 crc = calc_crc32(crc, (uint8 *)sys->mdc, SIZEOF_MDC_NV);
		 
		 SYS_TRACE("will save mdc crc=%x\r\n", crc);	
		 
		 sys->mdc_crc32 = crc;
		 
		 //DISABLE_INTERRUPT();
	   retval = eeprom_write(EPROM, SYS_NV_ADDRESS_MDC, (uint8 *)sys->mdc, SIZEOF_MDC_NV);
	   //ENABLE_INTERRUPT();
	   OSAL_ASSERT(retval > 0);
	 }

	 crc = 0;
	 
	 crc = calc_crc32(crc, sys, SIZEOF_SYS_NV - 8);
	
   SYS_TRACE("will save sys crc=%x\r\n", crc);	
	  
	 sys->sys_crc32 = crc;
	
	 //DISABLE_INTERRUPT();
	 retval = eeprom_write(EPROM, SYS_NV_ADDRESS, (uint8 *)sys, SIZEOF_SYS_NV);
	 //ENABLE_INTERRUPT();
	 OSAL_ASSERT(retval > 0);
}

static void sys_nv_set_default(sys_state_t *sys, uint8 type)
{
	
	if(0 == type){
		sys->src = SRC_COAX1;
		sys->src_num = SRC_CONSTANT_NUM;
		sys->master_vol = -20.0;
		sys->speak_output_mode = BYPASS;
		sys->line_output_mode = BYPASS;
		sys->ir_channel = 0;
		sys->is_auto_standby = true;
		sys->speaker_mode = SPEAKER_A_B;
		sys->balance_l = 0;
		sys->balance_r = 0;
		sys->bass_vol = 0;
		sys->treble_vol = 0;
	}else if(1 == type){
		mdc_t *p;
		uint8 j;
		
		p = (mdc_t *)sys->mdc;
		
		memset(p, 0, sizeof(mdc_t));
		
		p->mdc_slot_index = 0;
		p->mdc_slot_src_index = 0;
		
		for(j = 0; j < SLOT_NUM; j++){
			p->prev_slot[j].type = MDC_CARD_NUM;
		}
		
	}
	
	sys_database_save(sys, type);
	
}

static void sys_running_parameter_init(sys_state_t *sys)
{
	mdc_t *p;
	uint8 j;
	
	p = (mdc_t *)sys->mdc;
	
	
	sys->src_num = SRC_CONSTANT_NUM;
	sys->bitmapflag = 0;
	sys->protect_bitmap = 0;
	
	sys->mdc_test = NULL;
	
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	sys->previous_src = sys->src;
	sys->is_mute = 0;
	
	for(j = 0; j < SLOT_NUM; j++){
		p->slot[j].type = MDC_CARD_NUM;
	}
	
}


void sys_database_init(void)
{
	int retval;
	uint32 crc = 0;
	uint8 *p = MALLOC(sizeof(sys_state_t));
	OSAL_ASSERT(p != NULL);
	memset(p, 0, sizeof(sys_state_t));
	
  SYS_TRACE("will load %d bytes sys nv data from epprom\r\n", SIZEOF_SYS_NV);
	
	DISABLE_INTERRUPT();
	retval = eeprom_read(EPROM, SYS_NV_ADDRESS, p, SIZEOF_SYS_NV);
	ENABLE_INTERRUPT();
	OSAL_ASSERT(retval > 0);
	
	crc = calc_crc32(crc, p, SIZEOF_SYS_NV - 8);
	
	gSystem_t = (sys_state_t *)p;
	
	if(crc != gSystem_t->sys_crc32){
		SYS_TRACE("will set sys nv to default crc=%x, sys_crc=%x\r\n", crc, gSystem_t->sys_crc32);
		sys_nv_set_default(gSystem_t, 0);
		//gSystem_t->is_in_factory = 1;
	}else{
		SYS_TRACE("sys nv data load succuess, crc=%x\r\n", crc);
	}
	
/*************MDC nv data*****************/
	p = MALLOC(sizeof(mdc_t));
	OSAL_ASSERT(p != NULL);
	memset(p, 0, sizeof(mdc_t));
	
  SYS_TRACE("will load %d bytes mdc nv data from epprom\r\n", SIZEOF_MDC_NV);
	
	DISABLE_INTERRUPT();
	retval = eeprom_read(EPROM, SYS_NV_ADDRESS_MDC, p, SIZEOF_MDC_NV);
	ENABLE_INTERRUPT();
	OSAL_ASSERT(retval > 0);

	crc = 0;
	crc = calc_crc32(crc, p, SIZEOF_MDC_NV);
	
	gMdc = (mdc_t *)p;
	gSystem_t->mdc = (void *)gMdc;
	
	if(crc != gSystem_t->mdc_crc32){
		SYS_TRACE("will set mdc nv to default crc=%x, mdc_crc=%x\r\n", crc, gSystem_t->mdc_crc32);
		sys_nv_set_default(gSystem_t, 1);
	}else{
		SYS_TRACE("mdc nv data load succuess\r\n");
	}
	
	sys_running_parameter_init(gSystem_t);
}

static void sys_hw_power_on(void)
{
	AC_STANDBY(1);
	//POWER_EN01(1);
	//PS_EN(1);
	DC5V_EN(1);
	DC3V_EN(1);
	
	HC595_PS_EN(1);
	
	BT_POWER_ON;	
	BT_VREG(1);
	
	bsp_delay_ms(500);
	
	BT_RST(0);
	bsp_delay_ms(200); 
  BT_RST(1);	
}

static void sys_hw_power_off(void)
{
	SYS_TRACE("power off\r\n");

	
	SPEAKER1_MUTE_ON();
	SPEAKER2_MUTE_ON();
	
	bsp_delay_ms(100);
	
	AMP_MUTE_ON();
	AMP_MUTE2_ON();
	
	
	HP_MUTE_ON();
	LINE_MUTE_ON();
	
	bsp_delay_ms(500);
	
	AC_STANDBY(0);
//	POWER_EN01(0);
	//PS_EN(0);
	HC595_PS_EN(0);
	DC5V_EN(0);
	DC3V_EN(0);
	BT_POWER_OFF;	
	BT_VREG(0);
}

static void sys_mdc_power_on(sys_state_t *sys)
{
	uint8 j;
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	for(j = 0; j < SLOT_NUM; j++){
		switch(p->slot[j].type){
			case MDC_ANA:
			  ana_set_power((mdc_slot_t)j);	
			  bsp_delay_ms(4000);
				ana_init((mdc_slot_t)j);
				break;
			
			case MDC_USB:
				mdc_power_set((mdc_slot_t)j,ON);
	      bsp_delay_ms(500);
				mdc_usb_init((mdc_slot_t)j, p->slot_src_idx[j]);
			  osal_start_timerEx(gMdcTaskID, MDC_POLL_MSG,  100);		
				break;
			
			case MDC_DIO:
				mdc_dio_init((mdc_slot_t)j);
				break;
			
			case MDC_BLUOS:
				blu_hw_init((mdc_slot_t)j, p->slot_src_idx[j]);
			  bsp_delay_ms(500);
			  mdc_power_set((mdc_slot_t)j,ON);
        osal_start_timerEx(gMdcTaskID, MDC_POLL_MSG,  100);		
				break;
			
			case MDC_HDMI_v2:
				mdc_power_set((mdc_slot_t)j,ON);
	      bsp_delay_ms(500);
				hdmi_V2_init((mdc_slot_t)j);
			  bsp_delay_ms(500);
			  hdmi_V2_version_get((mdc_slot_t)j, &(p->slot[j].ver));
			  bsp_delay_ms(500);
			  osal_start_timerEx(gMdcTaskID, MDC_POLL_MSG,  100);		
				break;
			
			case MDC_HDMI:
				mdc_power_set((mdc_slot_t)j,ON);
	      bsp_delay_ms(500);
			  hdmi_init((mdc_slot_t)j);
			  hdmi_version_get((mdc_slot_t)j, &(p->slot[j].ver));
				break;
			
			default:
				break;
		}
	}
	
}

void sys_auto_standby_poll(sys_state_t *sys)
{
	src_list_t *p = gSrcListHead;
	 
	if(sys->is_auto_standby == false || sys->status != STATUS_WORKING || IS_TRIGGER_IN) return;
	
	while(p){
		/*since in computer source, we can't know whether it play or not, so we will not auto standby*/
		if((p->id == sys->src) && (!strstr(p->src_name, "Computer"))){
			if(p->state != SRC_STATE_PLAY){
				 if(false == IS_Flag_Valid(AUTO_STANDBY_TIMER_LOCK)){
						osal_start_timerEx(gSysTaskID, AUTO_STANDBY_MSG,  AUTO_STANDBY_TIMEOUT);		
					  Register_Bitmap_Flag(AUTO_STANDBY_TIMER_LOCK);
				 }
			}else if(true == IS_Flag_Valid(AUTO_STANDBY_TIMER_LOCK)){
				osal_stop_timerEx(gSysTaskID, AUTO_STANDBY_MSG);
				Deregister_Bitmap_Flag(AUTO_STANDBY_TIMER_LOCK);
			}
			break;
		}
		p = p->next;
	}
	
}


static void sys_input_init(sys_state_t *sys)
{
	src4382_init();
	
	pcm1795_init();
	
	sys_bass_vol_set(sys);
	
	sys_treble_vol_set(sys);
}

static void sys_output_control(sys_state_t *sys)
{
	//SPEAKER_SUB_OUTPUT();
	if(sys->speak_output_mode == HIGHPASS_OUTPUT){
		SPEAKER_SUB_OUTPUT();
	}else{
		SPEAKER_LFE_OUTPUT();
	}
	
  if(sys->line_output_mode == SUB_OUTPUT){
		LINE_SUB_OUT();
	}else{
		LINE_BYPASS();
	}	
}

static void do_power_on_later_handler(void)
{
	/*during initing, it may be powered off*/
	if(gSystem_t->status != STAUS_INITING) return;
	
	if(IS_HP_INSERT_IN){
		  SPEAKER1_MUTE_ON();
	    SPEAKER2_MUTE_ON();

		  LINE_MUTE_ON();
			
		  HP_MUTE_OFF();
			
		}else{
			
			HP_MUTE_ON();
			
			AMP_MUTE_OFF();
		  AMP_MUTE2_OFF();
			
			bsp_delay_ms(500);
		  
			if(gSystem_t->speaker_mode == SPEAKER_A){
				SPEAKER1_MUTE_OFF();
			}else if(gSystem_t->speaker_mode == SPEAKER_B){
				SPEAKER2_MUTE_OFF();
			}else if(gSystem_t->speaker_mode == SPEAKER_A_B){
				SPEAKER1_MUTE_OFF();
				SPEAKER2_MUTE_OFF();
			}

		  LINE_MUTE_OFF();
			
		}
		
		
		ui_led_control(LED_BIUE);
		SYS_TRACE("power check pass...\r\n");
		gSystem_t->status = STATUS_WORKING;
		
		display_menu_jump(SOURCE_MENU);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		
		
		/*start to check auto standby if auto standby is on*/
		//if(gSystem_t->is_auto_standby == true){
			osal_start_reload_timer(gSTTID, AUTO_STANDBY_POLL_MSG,  500);
		//}
	
}

static void show_protection_infor(void)
{
	if(IS_AC_ERROR){
		Send_DisplayTask_Msg(DIS_JUMP, AC_ERROR_MENU);
	}else if(IS_DC_ERROR){
		Send_DisplayTask_Msg(DIS_JUMP, DC_ERROR_MENU);
	}else if(IS_OVER_TEMP){
		Send_DisplayTask_Msg(DIS_JUMP, OVER_TEMP_MENU);
	}
}
static void sys_hw_power_check(void)
{
	if(IS_AC_ERROR | IS_DC_ERROR | IS_OVER_TEMP){
		SYS_TRACE("in sys_hw_power_check, AC=%d,DC=%d, OT=%d\r\n", IS_AC_ERROR, IS_DC_ERROR, IS_OVER_TEMP);
		//HC595_PS_EN(1);
		osal_start_timerEx(gSysTaskID, SYS_POWER_POLL_MSG,  500);
		
		/*show protection infor on the display*/
	  show_protection_infor();
		
	}else{
		
		if(gSystem_t->master_vol > -20){
			gSystem_t->master_vol = -20;
		}
		
		sys_master_vol_set(gSystem_t);
		sys_output_control(gSystem_t);
		
		
		osal_start_timerEx(gSysTaskID, SYS_POWER_ON_LATER_MSG,  6000);

	}
	

}

static void sys_power_off_handler(sys_state_t *sys)
{
	ui_led_control(LED_ORANGE);
	
	sys_hw_power_off();
	
	mdc_power_set(SLOT_NUM, OFF);
	
	/*dimer the lcd*/
	GPIO_PIN_SET(LCD_PWM_PORT, LCD_PWM_PIN, 0);
	
	/*clear up something and update some state*/
	do_delect_file_list(&gFileListHead[0], &gCurFile[0]);
	do_delect_file_list(&gFileListHead[1], &gCurFile[1]);
	
	sys_update_all_src_state(SRC_STATE_IDEL);
	
	/*clear for auto standby check*/
	Deregister_Bitmap_Flag(AUTO_STANDBY_TIMER_LOCK);	
	
	sys->status = STATUS_STANDBY;
}

static void sys_power_on_handler(sys_state_t *sys)
{
	sys->status = STAUS_INITING;
	
	ui_led_control(LED_RED);
	ui_init();
	
	sys_hw_power_on( );
	
	bsp_delay_ms(500);
	lcd_init();
	
	update_all_protect_state();

	
	GPIO_PIN_SET(LCD_PWM_PORT, LCD_PWM_PIN, 1);
	
	sys_mdc_power_on(sys);
	
	sys_input_init(sys);
	
	sys_input_select(sys);
	
	/*push out Uart Msg*/
	do_main_msg_handler(sys);
	
	sys_hw_power_check();
	
}

void sys_src_next_handler(sys_state_t *sys)
{
	if(gCurSrc){
		if(gCurSrc->next){
			gCurSrc = gCurSrc->next;
		}else{
			gCurSrc = gSrcListHead;
		}
		SYS_TRACE("get next src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}else{
		gCurSrc = gSrcListHead;
		SYS_TRACE("get next src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}
	
	sys->src = (src_t)gCurSrc->id;
	
	/*special handler for bt*/
	if(gCurSrc->id == (SRC_BT + 1)){
		if(gCurSrc->prev->state == SRC_STATE_PLAY){
       sys_bt_play_pause();			
		}
		
	}
	
	fix_pop_begin();
	
	//src4382_SRC_refrence_change();
	
	//src4382_PortB_Mute_Control(ON);
  //sys_input_select(sys);	
	/*clear for auto standby check*/
	Deregister_Bitmap_Flag(AUTO_STANDBY_TIMER_LOCK);
	
	display_menu_jump(SOURCE_MENU);
	Send_DisplayTask_Msg(DIS_UPDATE, 0);
	
	osal_start_timerEx(gSysTaskID, SYS_SRC_UPDATE_TIMEOUT_MSG, 1000);
	osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
}

void sys_src_prev_handler(sys_state_t *sys)
{
	src_list_t *p = gSrcListHead;
	
	if(gCurSrc){
		OSAL_ASSERT(gCurSrc->prev);
		gCurSrc = gCurSrc->prev;
		SYS_TRACE("get prev src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}else{
		gCurSrc = gSrcListHead;
		SYS_TRACE("get prev src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}
	
	sys->src = gCurSrc->id;
	
	/*special handler for bt*/
	if(gCurSrc->id == (SRC_BT - 1)){
		// BT_POWER_OFF;
		if(gCurSrc->next->state == SRC_STATE_PLAY){
       sys_bt_play_pause();			
		}
	}

	
	fix_pop_begin();
	
	
  //sys_input_select(sys);	
	/*clear for auto standby check*/
	Deregister_Bitmap_Flag(AUTO_STANDBY_TIMER_LOCK);
	
	display_menu_jump(SOURCE_MENU);
	Send_DisplayTask_Msg(DIS_UPDATE, 0);
	
	osal_start_timerEx(gSysTaskID, SYS_SRC_UPDATE_TIMEOUT_MSG, 1000);
	osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
}

static bool src_jump(sys_state_t *sys, uint8 id)
{
	src_list_t *p = gSrcListHead;
	
	while(p){
		if(p->id == id){
			gCurSrc = p;
			sys->src = id;
			return true;
		}
		
		p = p->next;
	}
	
	return false;
}

static void sys_src_select_handler(sys_state_t *sys, uint8 id)
{
	
	/*special handler for bt*/
	  if(gCurSrc->id == (SRC_BT) && gCurSrc->state == SRC_STATE_PLAY){
       // sys_bt_play_pause();		   
	  }
	
	/*when the id is valid we will jump successfully*/
	if(src_jump(sys, id) == true){
		
		sys_input_select(sys);	
	
	  display_menu_jump(SOURCE_MENU);
	  Send_DisplayTask_Msg(DIS_UPDATE, 0);
	
	  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		
	}

}


static void sys_config_menu_prev(sys_state_t *sys)
{
	OSAL_ASSERT(cur_menu != NULL);
	OSAL_ASSERT(cur_menu->prev != NULL);
	
	cur_menu = cur_menu->prev;
	/*special handler for hide MDC Firmware upgrade menu when no mec crad inserted*/
	if(cur_menu->id == FIRMWARE_UPGRADE_MENU && sys->src_num == SRC_CONSTANT_NUM){
		cur_menu = cur_menu->prev;
	}
	
	Send_DisplayTask_Msg(DIS_UPDATE, 0);
	
	osal_start_timerEx(gSysTaskID, SYS_CONFIG_MENU_BACK_MSG, SYS_CONFIG_BACK_TIMEOUT);
}

static void sys_config_menu_next(sys_state_t *sys)
{
	OSAL_ASSERT(cur_menu != NULL);
	
	if(cur_menu->next){
		cur_menu = cur_menu->next;
		/*special handler for hide MDC Firmware upgrade menu when no mec crad inserted*/
		if(cur_menu->id == FIRMWARE_UPGRADE_MENU && sys->src_num == SRC_CONSTANT_NUM){
			if(cur_menu->next)
				cur_menu = cur_menu->next;
			else
				display_menu_jump(OUTPUT_SELECT_MENU);
		}
		
	}else{
		display_menu_jump(OUTPUT_SELECT_MENU);
	}
	
	Send_DisplayTask_Msg(DIS_UPDATE, 0);
	
	osal_start_timerEx(gSysTaskID, SYS_CONFIG_MENU_BACK_MSG, SYS_CONFIG_BACK_TIMEOUT);
}

static void sys_up_key_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(cur_menu == NULL) return;
	
	if(cur_menu->id == USB_FILE_LIST_MENU){
     sys_usb_file_prev(p->mdc_slot_index);		
		 Send_DisplayTask_Msg(DIS_UPDATE, 1);
	}else if(cur_menu->id >= OUTPUT_SELECT_MENU){
		sys_config_menu_prev(sys);
	}
	
	
}

static void sys_down_key_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(cur_menu == NULL) return;
	
	if(cur_menu->id == USB_FILE_LIST_MENU){
		sys_usb_file_next(p->mdc_slot_index);
		
	}else if(cur_menu->id >= OUTPUT_SELECT_MENU){
		sys_config_menu_next(sys);
	}
	
}

static void sys_enter_key_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(cur_menu == NULL) return;
	
	if(cur_menu->id == USB_FILE_LIST_MENU){
		 mdc_usb_send_msg(p->mdc_slot_index, "Media.Play=%d\n", gCurFile[p->mdc_slot_index]->id);
		 Send_DisplayTask_Msg(DIS_JUMP, USB_READING_MENU);
	}else if(cur_menu->id == SOURCE_MENU || cur_menu->id == BT_STATUS_MENU){
     display_menu_jump(OUTPUT_SELECT_MENU);
		 Send_DisplayTask_Msg(DIS_UPDATE, 0);		 
	}else if(cur_menu->id >=  OUTPUT_SELECT_MENU){
		 display_menu_jump(SOURCE_MENU);
		 Send_DisplayTask_Msg(DIS_UPDATE, 0);		 
	}
	
}

void sys_output_mode_select(sys_state_t *sys)
{
	AMP_MUTE_ON();
	AMP_MUTE2_ON();
	bsp_delay_ms(100);
	osal_start_timerEx(gNADUartID, FIX_SPEAKER_OUT_MODE_CHANGE_POP_MSG,  FIX_SPEAKER_OUT_MODE_CHANGE_POP_TIMEOUT);

	if(sys->speak_output_mode == HIGHPASS_OUTPUT){
		SPEAKER_SUB_OUTPUT();
	}else{
		SPEAKER_LFE_OUTPUT();
	}
	
}

void sys_line_out_mode_select(sys_state_t *sys)
{
	LINE_MUTE_ON();
	bsp_delay_ms(100);
	osal_start_timerEx(gNADUartID, FIX_LINE_OUT_MODE_CHANGE_POP_MSG,  FIX_LINE_OUT_MODE_CHANGE_POP_TIMEOUT);
	
	if(sys->line_output_mode == SUB_OUTPUT){
		LINE_SUB_OUT();
	}else{
		LINE_BYPASS();
	}
	
}

static void sys_speaker_mode_select_handler(sys_state_t *sys)
{
	
	if(sys->speaker_mode == SPEAKER_A){
		SPEAKER1_MUTE_OFF();
		SPEAKER2_MUTE_ON();
	}else if(sys->speaker_mode == SPEAKER_B){
		SPEAKER1_MUTE_ON();
		SPEAKER2_MUTE_OFF();
	}else if(sys->speaker_mode == SPEAKER_A_B){
		SPEAKER1_MUTE_OFF();
		SPEAKER2_MUTE_OFF();
	}else if(sys->speaker_mode == SPEAKER_OFF){
		SPEAKER1_MUTE_ON();
		SPEAKER2_MUTE_ON();
	}
	
}


static void sys_auto_stanby_onoff_handler(sys_state_t *sys)
{
  if(sys->is_auto_standby == false){
		osal_stop_timerEx(gSysTaskID, AUTO_STANDBY_MSG);
		Deregister_Bitmap_Flag(AUTO_STANDBY_TIMER_LOCK);
	}
}

static void sys_treble_vol_set(sys_state_t *sys)
{
	njw1194_treble_set(sys->treble_vol);
	
}

static void sys_bass_vol_set(sys_state_t *sys)
{
	njw1194_bass_set(sys->bass_vol);
}

static void sys_balance_set(sys_state_t *sys)
{
	sys_master_vol_set(sys);
}



static void sys_left_key_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(cur_menu == NULL) return;
	
	if(cur_menu->id >= OUTPUT_SELECT_MENU){
		osal_start_timerEx(gSysTaskID, SYS_CONFIG_MENU_BACK_MSG, SYS_CONFIG_BACK_TIMEOUT);
	}
	
	switch(cur_menu->id){
		
		case OUTPUT_SELECT_MENU:
			sys->speak_output_mode = (sys->speak_output_mode == BYPASS) ? HIGHPASS_OUTPUT : BYPASS;
		 // sys_output_mode_select(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, SPEAKER_OUT_NEED_UPDATE);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
			break;
		
		case LINE_OUT_SELECT_MENU:
			sys->line_output_mode = (sys->line_output_mode == BYPASS) ? SUB_OUTPUT : BYPASS;
		 // sys_line_out_mode_select(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, LINE_OUT_NEED_UPDATE);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
			break;
		
		case USB_FILE_LIST_MENU:
			mdc_usb_send_msg(p->mdc_slot_index, "Media.Action=AscendDir\n");
		  Send_DisplayTask_Msg(DIS_JUMP, USB_READING_MENU);
		  osal_start_timerEx(gSysTaskID, SYS_USB_READING_TIMEOUT_MSG, 5000);
			break;
		
		case USB_PLAY_MENU:
			display_menu_jump(USB_FILE_LIST_MENU);
	    Send_DisplayTask_Msg(DIS_UPDATE, 2);
			break;
		
		case IR_CHANNEL_SELECT_MENU:
			sys->ir_channel = (sys->ir_channel == 0) ? 3 : (sys->ir_channel - 1);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
			break;
		
		case AUTO_STANDBY_MENU:
			sys->is_auto_standby = (sys->is_auto_standby == false) ? true : false;
		  sys_auto_stanby_onoff_handler(sys);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case SPEAKE_MODE_SELECT_MENU:
			sys->speaker_mode = (sys->speaker_mode == SPEAKER_A_B) ? SPEAKER_OFF : (speaker_mode_t)(sys->speaker_mode - 1);
		  sys_speaker_mode_select_handler(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case TREBEL_CONTROL_MENU:
			sys->treble_vol = (sys->treble_vol == SYS_TREBLE_VOL_MIN) ? SYS_TREBLE_VOL_MIN : (sys->treble_vol - 1);
		  sys_treble_vol_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case BASS_CONTROL_MENU:
			sys->bass_vol = (sys->bass_vol == SYS_BASS_VOL_MIN) ? SYS_BASS_VOL_MIN : (sys->bass_vol - 1);
		  sys_bass_vol_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case BALANCE_CONTROL_MENU:
			if(sys->balance_r < 0){
				sys->balance_r = (sys->balance_r == 0) ? 0 : (sys->balance_r + 1);
			}else{
				sys->balance_l = (sys->balance_l == SYS_BALANCE_MIN) ? SYS_BALANCE_MIN : (sys->balance_l - 1);
			}
				
			sys_balance_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
			
		case FIRMWARE_UPGRADE_MENU:

		{
			mdc_slot_t temp;
			
			temp = (gSlot == SLOT_1 ? SLOT_2 : SLOT_1);
			
			if(p->slot[temp].type != MDC_CARD_NUM){
				gSlot = temp;
				Send_DisplayTask_Msg(DIS_UPDATE, 0);
			}
		}
			break;
		
		default:
			break;
	}
}

static void sys_right_key_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(cur_menu == NULL) return;
	
	if(cur_menu->id >= OUTPUT_SELECT_MENU){
		osal_start_timerEx(gSysTaskID, SYS_CONFIG_MENU_BACK_MSG, SYS_CONFIG_BACK_TIMEOUT);
	}
	
	switch(cur_menu->id){
		
		case OUTPUT_SELECT_MENU:
			sys->speak_output_mode = (sys->speak_output_mode == BYPASS) ? HIGHPASS_OUTPUT : BYPASS;
		 // sys_output_mode_select(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, SPEAKER_OUT_NEED_UPDATE);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
			break;
		
		case LINE_OUT_SELECT_MENU:
			sys->line_output_mode = (sys->line_output_mode == BYPASS) ? SUB_OUTPUT : BYPASS;
		  //sys_line_out_mode_select(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, LINE_OUT_NEED_UPDATE);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
			break;
		
		case IR_CHANNEL_SELECT_MENU:
			sys->ir_channel = (sys->ir_channel == 3) ? 0 : (sys->ir_channel + 1);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
			break;
		
		case AUTO_STANDBY_MENU:
			sys->is_auto_standby = (sys->is_auto_standby == false) ? true : false;
		  sys_auto_stanby_onoff_handler(sys);
		  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case SPEAKE_MODE_SELECT_MENU:
			sys->speaker_mode = (sys->speaker_mode == SPEAKER_OFF) ? SPEAKER_A_B : (speaker_mode_t)(sys->speaker_mode + 1);
		  sys_speaker_mode_select_handler(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case TREBEL_CONTROL_MENU:
			sys->treble_vol = (sys->treble_vol == SYS_TREBLE_VOL_MAX) ? SYS_TREBLE_VOL_MAX : (sys->treble_vol + 1);
		  sys_treble_vol_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;

		case BASS_CONTROL_MENU:
			sys->bass_vol = (sys->bass_vol == SYS_BASS_VOL_MAX) ? SYS_BASS_VOL_MAX : (sys->bass_vol + 1);
		  sys_bass_vol_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case BALANCE_CONTROL_MENU:
		  	
		   if(sys->balance_l < 0){
				 sys->balance_l = (sys->balance_l == 0) ? 0 : (sys->balance_l + 1);
			 }else{
				 sys->balance_r = (sys->balance_r == SYS_BALANCE_MIN) ? SYS_BALANCE_MIN : (sys->balance_r - 1);
			 }
		
			
			sys_balance_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case USB_FILE_LIST_MENU:
			mdc_usb_send_msg(p->mdc_slot_index, "Media.Play=%d\n", gCurFile[p->mdc_slot_index]->id);
		  Send_DisplayTask_Msg(DIS_JUMP, USB_READING_MENU);
			break;
		
		case FIRMWARE_UPGRADE_MENU:
			{
				mdc_slot_t temp;
				
				temp = (gSlot == SLOT_1 ? SLOT_2 : SLOT_1);
				
				if(p->slot[temp].type != MDC_CARD_NUM){
					gSlot = temp;
					Send_DisplayTask_Msg(DIS_UPDATE, 0);
				}
		  }
			break;
		
		default:
			break;
	}
	
	
}

static void sys_bt_go_pairng(void)
{
	bt_send_byte(0x03);
	bt_send_byte(0x00);
	bt_send_byte(0x55);
	bt_send_byte(0xff);
}

static void sys_bt_play_pause(void)
{
	
	bt_send_byte(0x20);
	bt_send_byte(0x00);
	bt_send_byte(0x55);
	bt_send_byte(0xff);
	
	SYS_TRACE("sys_bt_play_pause\r\n");
}

extern void ConfigureUSBInterface(void);

extern uint8 gUpgradeTaskID;

static void _mdc_firmware_upgrade_handler(sys_state_t *sys)
{
	
		mdc_t *p;
		
		p = (mdc_t *)sys->mdc;
		
		if(p->slot[gSlot].type != MDC_USB){
		  dis_simple_print(LCD_LINE_2, "Not Support");
			return ;
		}
	
	  dis_simple_print(LCD_LINE_1, "MDC USB Upgrade");
		dis_simple_print(LCD_LINE_2, "Initializing...");
		
		/*mdc card firmware upgrade*/
		ConfigureUSBInterface();
	
		mdc_power_set(gSlot, OFF);
		
		bsp_delay_ms(100);
		
		mdc_power_set(gSlot, ON);
		
		osal_start_timerEx(gUpgradeTaskID, UPGRADE_POLL_SMG, 100);
	
}

static void sys_enter_long_down_handler(sys_state_t *sys)
{
	
	if(cur_menu->id == FIRMWARE_UPGRADE_MENU){
		
		_mdc_firmware_upgrade_handler(sys);
		
		return ;
	}
	
	
	if(gCurSrc->id == SRC_BT){
		sys_bt_go_pairng();
	}
}

void sys_factory_reset(sys_state_t *sys)
{
	SYS_TRACE("system factory reset...\r\n");
	
	SPEAKER1_MUTE_ON();
	SPEAKER2_MUTE_ON();
	
	LINE_MUTE_ON();
	AMP_MUTE_ON();
	AMP_MUTE2_ON();
	
	display_factory_menu();
	
	/*set BT to factory mode*/
	bt_clear_all_paring_list();
	
	mdc_power_set(SLOT_NUM, OFF);
	
	sys_src_list_detach(&gSrcListHead);
	
	/*clear up something and update some state*/
	do_delect_file_list(&gFileListHead[0], &gCurFile[0]);
	do_delect_file_list(&gFileListHead[1], &gCurFile[1]);
	
	sys_update_all_src_state(SRC_STATE_IDEL);
	
	/*clear for auto standby check*/
	Deregister_Bitmap_Flag(AUTO_STANDBY_TIMER_LOCK);	
	
  sys_nv_set_default(sys, 0);	
	sys_nv_set_default(sys, 1);
	
	sys_running_parameter_init(sys);
	
	mdc_init();
	
	sys_src_load_cur(gSystem_t, &gCurSrc);
	
	sys_mdc_power_on(sys);

	sys_input_select(sys);
	
	sys_master_vol_set(sys);
	sys_output_control(sys);
	
	bsp_delay_ms(500);
	
	if(!IS_HP_INSERT_IN){
		LINE_MUTE_OFF();
		AMP_MUTE_OFF();
		AMP_MUTE2_OFF();
		
		bsp_delay_ms(500);
		
		if(sys->speaker_mode == SPEAKER_A){
				SPEAKER1_MUTE_OFF();
			}else if(sys->speaker_mode == SPEAKER_B){
				SPEAKER2_MUTE_OFF();
			}else if(sys->speaker_mode == SPEAKER_A_B){
				SPEAKER1_MUTE_OFF();
				SPEAKER2_MUTE_OFF();
			}
	}
	
	display_menu_jump(SOURCE_MENU);
	Send_DisplayTask_Msg(DIS_UPDATE, 0);
	
}


static void sys_src_next_key_long_down_handler(sys_state_t *sys)
{
	if(osal_key_is_down(KEY_SRC_PREV) == true){
		sys_factory_reset(sys);
	}
	
}

extern void lcd_bus_test(void);

static void keyboard_msg_handler(sys_state_t *sys, key_value_t key)
{
	static uint8 cnt = 0;
	
	SYS_TRACE("get key = [%d]\r\n", key);
	
	if(sys->status != STATUS_WORKING){ 
		SYS_TRACE("sys->status != STATUS_WORKING, invalid key\r\n");
		return;
	}

	switch(key){
		
		case KEY_SRC_NEXT:
			sys_src_next_handler(sys);
			break;
		
		case KEY_SRC_PREV:
			sys_src_prev_handler(sys);
			break;
		
		case KEY_SRC_NEXT_LONG:
			sys_src_next_key_long_down_handler(sys);
			break;
		
		case KEY_UP:
			sys_up_key_handler(sys);
			break;
		
		case KEY_DOWN:
			sys_down_key_handler(sys);
			break;
		
		case KEY_ENTER:
			sys_enter_key_handler(sys);
			break;
		
		case KEY_LEFT:
			sys_left_key_handler(sys);
			break;
		
		case KEY_RIGHT:
			sys_right_key_handler(sys);
			break;
		
		case KEY_ENTER_LONG:
			sys_enter_long_down_handler(sys);
			break;
		
		
		default:
			break;
	}
	
}

static void sys_ir_on_handler(sys_state_t *sys)
{
	if(IS_TRIGGER_IN) return;
	
	if(sys->status != STATUS_WORKING){
			sys_power_on_handler(sys);
		}
}

static void sys_ir_off_handler(sys_state_t *sys)
{
	if(IS_TRIGGER_IN) return;
	
	if(sys->status == STATUS_WORKING){
			sys_power_off_handler(sys);
		}
	
}

static void sys_mute_control(sys_state_t *sys)
{
	sys->is_mute = (sys->is_mute == false)? true : false;
	
	sys_vol_control_handler(sys, DIR_EQU, 0);
}

static void sys_music_pause_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
		
	if(gCurSrc->state == SRC_STATE_PLAY){
			if(strstr(gCurSrc->src_name, "USB")){
				mdc_usb_send_msg(p->mdc_slot_index, "Media.Action=Pause\n");
			}
	}
	
}

static void sys_music_play_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
		
	if(gCurSrc->state == SRC_STATE_PAUSE){
			if(strstr(gCurSrc->src_name, "USB")){
				mdc_usb_send_msg(p->mdc_slot_index, "Media.Action=Play\n");
			}
	}
	
}

static void sys_show_config_menu_handler(sys_state_t *sys)
{
	display_menu_jump(OUTPUT_SELECT_MENU);
	Send_DisplayTask_Msg(DIS_UPDATE, 0);		
	osal_start_timerEx(gSysTaskID, SYS_CONFIG_MENU_BACK_MSG, SYS_CONFIG_BACK_TIMEOUT);
}

static void sys_ir_command_handler(sys_state_t *sys, ir_commond_t cmd)
{
	SYS_TRACE("ir_get_commond:%d\r\n", cmd);
	
	switch(cmd){
		
		case IR_ON:
			sys_ir_on_handler(sys);
			break;
		
		case IR_OFF:
			sys_ir_off_handler(sys);
			break;
		
		case IR_SOURCE_DOWN:
			sys_src_next_handler(sys);
			break;
		
		case IR_SOURCE_UP:
			sys_src_prev_handler(sys);
			break;
		
		case IR_VOL_UP:
			sys_vol_control_handler(sys, DIR_UP, 0);
		  /*HDMI V2*/
		  Send_MdcTask_Msg(MDC_VOLUME_MSG, 0);
			break;
		
		case IR_VOL_DOWN:
			sys_vol_control_handler(sys, DIR_DOWN, 0);
		  /*HDMI V2*/
		  Send_MdcTask_Msg(MDC_VOLUME_MSG, 0);
			break;
		
		case IR_DOWN:
			sys_down_key_handler(sys);
			break;
		
		case IR_CENTER:
			sys_enter_key_handler(sys);
			break;
		
		case IR_LFET:
			sys_left_key_handler(sys);
			break;
		
		case IR_UP:
			sys_up_key_handler(sys);
			break;
		
		case IR_RIGHT:
			sys_right_key_handler(sys);
			break;
		
		case IR_MUTE:
			sys_mute_control(sys);
			break;
		
		case IR_PAUSE:
			sys_music_pause_handler(sys);
			break;
		
		case IR_PLAY:
			sys_music_play_handler(sys);
			break;
		
		case IR_MENU:
			sys_show_config_menu_handler(sys);
			break;
		
		case IR_NUM1:
		case IR_NUM2:
		case IR_NUM3:
		case IR_NUM4:
		case IR_NUM5:
		case IR_NUM6:
		case IR_NUM7:
		case IR_NUM8:
		case IR_NUM9:
		case IR_NUM10:
			sys_src_select_handler(sys, (cmd - IR_NUM1));
			break;
		
		default:
			break;
	}
	
}

static void power_key_msg_handler(sys_state_t *sys, key_value_t key)
{
	if(IS_TRIGGER_IN) return;
	
	if(key == KEY_STANDBY){
		if(sys->status != STATUS_WORKING && sys->status != STAUS_INITING){
			sys_power_on_handler(sys);
		}else{
			sys_power_off_handler(sys);
		}
	}
}

static void sys_vol_control_handler(sys_state_t *sys, uint8 dir, uint8 step)
{
	mdc_t *p = (mdc_t *)sys->mdc;
	
	/*vol up*/
	if(dir == DIR_UP){
		if(sys->master_vol < MASTER_VOL_INDEX_MAX){
			sys->master_vol += (0.5 + step);
		}
		
		if(sys->master_vol > MASTER_VOL_INDEX_MAX){
			sys->master_vol = MASTER_VOL_INDEX_MAX;
		}
		
		sys->is_mute = false;
	}else if(dir == DIR_DOWN){
		if(sys->master_vol > MASTER_VOL_INDEX_MIN){
			sys->master_vol -= (0.5 + step);
			
			if(sys->master_vol < MASTER_VOL_INDEX_MIN){
				sys->master_vol = MASTER_VOL_INDEX_MIN;
			}
			
			sys->is_mute = false;
		}else{
			sys->is_mute = true;
		}

	}
	
	sys_master_vol_set(sys);
  if(strstr(gCurSrc->src_name, "BluOS")){
		osal_start_timerEx(gSysTaskID, SEND_BLUOS_RE_MSG, 300);
	}		
	
		Send_DisplayTask_Msg(DIS_JUMP, SOURCE_MENU);
		Register_Bitmap_Flag(DISPLAY_UPDATE_LOCK);
		osal_start_timerEx(gDisTaskID, VOL_DIS_BACK_MSG,  VOL_MENU_DIS_TIMEOUT);
	  osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, DATABASE_SAVE_TIMEOUT);
	
}

static void sys_send_bluos_response_msg(sys_state_t *sys)
{
	mdc_t *p = (mdc_t *)sys->mdc;
	
	SYS_TRACE("sys_send_bluos_response_msg:slot[%d]\r\n", p->mdc_slot_index);
	
	blu_send_msg(p->mdc_slot_index, "Main.Volume=%.1f\n", sys->master_vol);
	blu_send_msg(p->mdc_slot_index, "Main.Volume=%.1f\n", sys->master_vol);
}

static void sys_auto_stanby_handler(sys_state_t *sys)
{
	if(sys->is_auto_standby == false || IS_TRIGGER_IN) return;
	
	SYS_TRACE("AUTO STANDBY TIMEOUT\r\n");
	
  if(sys->status != STATUS_STANDBY)	{
		sys_power_off_handler(sys);
	}
	
  
}

static void sys_usb_file_list_update_handler(sys_state_t *sys)
{
	mdc_t *p = (mdc_t *)sys->mdc;
	
	if(gCurFile[p->mdc_slot_index]->next){
		gCurFile[p->mdc_slot_index]= gCurFile[p->mdc_slot_index]->next;	
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}else{
		SYS_TRACE("resend USB_FILE_LIST_UPDATE_MSG\r\n");
		osal_start_timerEx(gSysTaskID, USB_FILE_LIST_UPDATE_MSG,  500);
	}
}

static void sys_usb_reading_timeout_handler(void)
{
	if(gCurMenu == USB_READING_MENU){
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}
	
}

uint16 SysEventLoop(uint8 task_id, uint16 events)
{
	   Sys_Msg_t *msg = NULL;
	
	   if(events & SYS_EVENT_MSG)
		 {
			  msg = (Sys_Msg_t *)osal_msg_receive(task_id);
			 
			  while(msg)
				{
					 switch(msg->hdr.event)
					 {
						    case SYS_KEYBOARD_MEG:
									keyboard_msg_handler(gSystem_t, (key_value_t)(msg->value));
									break;
								
								case SYS_POWER_MSG:
									power_key_msg_handler(gSystem_t, (key_value_t)(msg->value));
									break;
								
								case SYS_DATABASE_SAVE_MSG:
									sys_database_save(gSystem_t, msg->value);
									break;
								
								case SYS_VOL_UP:
									sys_vol_control_handler(gSystem_t,DIR_UP, msg->value);
								  /*HDMI V2*/
									//Send_MdcTask_Msg(MDC_VOLUME_MSG, 0);
								  osal_start_timerEx(gMdcTaskID, HDMI_V2_VOLUME_PUSH_MSG,  1000);
								  
									break;
								
								case SYS_VOL_DOWN:
									sys_vol_control_handler(gSystem_t,DIR_DOWN, msg->value);
								  /*HDMI V2*/
									//Send_MdcTask_Msg(MDC_VOLUME_MSG, 0);
								  osal_start_timerEx(gMdcTaskID, HDMI_V2_VOLUME_PUSH_MSG,  1000);
									break;
								
								case SYS_VOL_SET:
									sys_vol_control_handler(gSystem_t,DIR_EQU, 0);
									break;
								
								case SYS_IR_MSG:
									sys_ir_command_handler(gSystem_t, (ir_commond_t)(msg->value));
									break;
								
								default:
									break;
					 }
					 
					 osal_msg_deallocate((uint8 *) msg);
					 
					 msg = (Sys_Msg_t *)osal_msg_receive(task_id);
				}
				
				return (events ^ SYS_EVENT_MSG);
		 }
		 
		 if(events & SYS_POWER_POLL_MSG){
			 
			 sys_hw_power_check();
			 
			 return (events ^ SYS_POWER_POLL_MSG);
		 }
		 
		 if(events & DATABASE_SAVE_MSG){
			 sys_database_save(gSystem_t, 0);
			 return (events ^ DATABASE_SAVE_MSG);
		 }
		 
		 if(events & AUTO_STANDBY_MSG){
			 
			 sys_auto_stanby_handler(gSystem_t);
			 
			 return(events ^ AUTO_STANDBY_MSG);
		 }
		 
		 if(events & SEND_BLUOS_RE_MSG){
			 sys_send_bluos_response_msg(gSystem_t);
			 return (events ^ SEND_BLUOS_RE_MSG);
		 }
		 
		 if(events & USB_FILE_LIST_UPDATE_MSG){
			 sys_usb_file_list_update_handler(gSystem_t);
			 return(events ^ USB_FILE_LIST_UPDATE_MSG);
		 }
		 
		 if(events & SYS_USB_READING_TIMEOUT_MSG){
			 
			 sys_usb_reading_timeout_handler();
			 return (events ^ SYS_USB_READING_TIMEOUT_MSG);
		 }
		 
		 if(events & SYS_SRC_UPDATE_TIMEOUT_MSG){
			 
			 sys_input_select(gSystem_t);	
			 return(events ^ SYS_SRC_UPDATE_TIMEOUT_MSG);
		 }
		 
		 if(events & SYS_POWER_ON_LATER_MSG){
			 
			 do_power_on_later_handler();
			 return(events ^ SYS_POWER_ON_LATER_MSG);
		 }
		 
		 return 0;
 }


void Send_SysTask_Msg(sys_event_t event, uint8 value)
{
	   uint8 result = 0;
		 
	   gSys = (Sys_Msg_t *)osal_msg_allocate(sizeof(Sys_Msg_t));
		 if(NULL == gSys)
     {
			 SYS_TRACE("no memory for sys msg\r\n");
			 return;
		 }
		 gSys->hdr.event = (uint8)event;	
		 gSys->value = value;
	 
		 result = osal_msg_send(gSysTaskID,(uint8 *)gSys);
		 if(result != SUCCESS)
		 {
			  SYS_TRACE("send sys msg fail\r\n");
		 }
}

static void sys_mdc_input_select(sys_state_t *sys)
{
	mdc_t *p;
	uint8 j;
	
	p = (mdc_t *)sys->mdc;
	
	SYS_TRACE("sys_mdc_input_select: src_num=%d\r\n", sys->src_num);
	
	for(j = 0; j < SLOT_NUM; j++){
		if(p->slot[j].type != MDC_CARD_NUM){
			if(sys->src >= p->slot_src_idx[j] && sys->src < (p->slot_src_idx[j] + p->slot[j].max_input)){
				switch(p->slot[j].type){
					case MDC_ANA:
						mdc_ana_input_select((mdc_slot_t)j, (ana_input_t)(sys->src - p->slot_src_idx[j]));
						break;
					
					case MDC_USB:
						mdc_usb_input_select((mdc_slot_t)j, (usb_input_t)(sys->src - p->slot_src_idx[j]), p->slot_src_idx[j]);
					  p->mdc_slot_src_index = sys->src - p->slot_src_idx[j];
			      p->mdc_slot_index = j;
						break;
					
					case MDC_DIO:
						SYS_TRACE("mdc_dio_input_select:%d\r\n", sys->src - p->slot_src_idx[j]);
						mdc_dio_input_select((mdc_slot_t)j, (dio_input_t)(sys->src - p->slot_src_idx[j]));
						break;
					
					case MDC_BLUOS: 
						mdc_usb_bus_select((mdc_slot_t)j);
					  p->mdc_slot_src_index = sys->src - p->slot_src_idx[j];
			      p->mdc_slot_index = j;
						break;
					
					case MDC_HDMI_v2:
						SYS_TRACE("sys_mdc_input_select:MDC_HDMI_v2:%d\r\n", (hdmi_v2_input_t)(sys->src - p->slot_src_idx[j]));
						hdmi_V2_input_select((mdc_slot_t)j, (hdmi_v2_input_t)(sys->src - p->slot_src_idx[j]));
					  p->mdc_slot_src_index = sys->src - p->slot_src_idx[j];
			      p->mdc_slot_index = j;
						break;
					
					case MDC_HDMI:
						hdmi_input_select((mdc_slot_t)j, (hdmi_input_t)(sys->src - p->slot_src_idx[j]));
					  p->mdc_slot_src_index = sys->src - p->slot_src_idx[j];
			      p->mdc_slot_index = j;
						break;
					
					default:
						break;
				}
				
				/*select mdc iis output*/
		    mdc_iis_select((mdc_slot_t)j);
			}
			
		}
	
	}
}

void fix_pop_begin(void)
{
	njw1194_mute();
	bsp_delay_ms(50);
	LINE_MUTE_ON();
	AMP_MUTE_ON();
	AMP_MUTE2_ON();
	osal_start_timerEx(gNADUartID, FIX_SOURCE_CHANGE_POP_MSG,  FIX_SRC_CHANGE_POP_TIMEOUT);
}

void fix_pop_end(void)
{

		//bsp_delay_ms(800);
//#ifdef NAD_C378	
//		if(gSystem_t->speaker_mode == SPEAKER_A){
//			  SPEAKER1_MUTE_OFF();
//		}else if(gSystem_t->speaker_mode == SPEAKER_B){
//			  SPEAKER2_MUTE_OFF();
//		}else if(gSystem_t->speaker_mode == SPEAKER_A_B){
//			  SPEAKER1_MUTE_OFF();
//			  SPEAKER2_MUTE_OFF();
//		}
//#else
//	      SPEAKER1_MUTE_OFF();
//#endif
//				LINE_MUTE_OFF();
		
	 sys_master_vol_set(gSystem_t);
	
	 if(!IS_HP_INSERT_IN){ 
      AMP_MUTE_OFF();
	    AMP_MUTE2_OFF();
	    LINE_MUTE_OFF();
	 }
	
}

static void sys_constant_input_select(sys_state_t *sys)
{
  switch(sys->src){
		
		case SRC_COAX1:
			/*select SRC input*/
			src4382_SRC_input_select(SRC4382_SRC_RX3);
			src4382_PortB_input_select(SRC4382_SRC_RX3);
		
		  /*select njw1194 input*/
      njw1194_input_select(FROM_DAC);
		
			break;
		
		case SRC_COAX2:
			/*select SRC input*/
			src4382_SRC_input_select(SRC4382_SRC_RX4);
			src4382_PortB_input_select(SRC4382_SRC_RX4);
		
		  /*select njw1194 input*/
      njw1194_input_select(FROM_DAC);
			break;
		
		case SRC_OPT1:
			/*select SRC input*/
			src4382_SRC_input_select(SRC4382_SRC_RX1);
			src4382_PortB_input_select(SRC4382_SRC_RX1);
		  /*select njw1194 input*/
      njw1194_input_select(FROM_DAC);
			break;
		
		case SRC_OPT2:
			/*select SRC input*/
			src4382_SRC_input_select(SRC4382_SRC_RX2);
			src4382_PortB_input_select(SRC4382_SRC_RX2);
		
		  /*select njw1194 input*/
      njw1194_input_select(FROM_DAC);
			 
			break;
		
		case SRC_LINE:
			SYS_TRACE("njw1194_input_select:LINE\r\n");

			njw1194_input_select(FROM_LINE);
			 
			break;
		
		case SRC_MM:
			SYS_TRACE("njw1194_input_select:MM\r\n");
		
			njw1194_input_select(FROM_MM);
			break;
		
		case SRC_BT:		
		
			src4382_SRC_input_select(SRC4382_SRC_IIS_BT);
		  src4382_PortB_input_select(SRC4382_SRC_IIS_BT);
		  njw1194_input_select(FROM_DAC);
		
		  if(sys_get_src_state("Bluetooth") == SRC_STATE_IDEL){
				/*TO fix continue pop noise issue*/
				src4382_PortB_Mute_Control(ON);
			}
		
			break;
		
		default:
			break;
	}

}


void sys_master_vol_set(sys_state_t *sys)
{
	uint8 value;
	
  if(sys->master_vol == MASTER_VOL_INDEX_MIN || sys->is_mute == true){
		value = 0xff;
	
	}else{
		value = NJW1194_0DB_GAIN_VALUE - (int)(sys->master_vol * 2) ;
	}
	
	SYS_TRACE("sys_master_vol_set:[%.1f] [%2x]\r\n", sys->master_vol, value);
	
//	njw1194_volume_set(value);
	
	/*changed since v0.1.7 due to add balance feature*/
	if(sys->balance_l < 0){
		njw1194_volume_l_set(value - (sys->balance_l << 1));
		njw1194_volume_r_set(value);
	}else{
		njw1194_volume_r_set(value - (sys->balance_r << 1));
		njw1194_volume_l_set(value);
	}
}


void sys_input_select(sys_state_t *sys)
{
	/*TO fix continue pop noise*/
	src4382_PortB_Mute_Control(ON);
	
	if(sys->src < SRC_CONSTANT_NUM){
		/*select onboard input*/
		SYS_TRACE("sys_input_select:select constant srouce[%d]\r\n", sys->src);
		sys_constant_input_select(sys);
	}else{
		/*select MDC input*/
		sys_mdc_input_select(sys);
		
		/*select SRC input*/
		src4382_SRC_input_select(SRC4382_SRC_IIS_MDC);
		src4382_PortB_input_select(SRC4382_SRC_IIS_MDC);
		/*select njw1194 input*/
		njw1194_input_select(FROM_DAC);
		
	}
	
	
	/*update the previous src*/
	sys->previous_src = sys->src;
}

static void src_list_add_next(src_list_t **head, uint8 id, const char *name)
{
	src_list_t *list;
	src_list_t *p = *head;
	
	list = (src_list_t *)MALLOC(sizeof(src_list_t));
	
	OSAL_ASSERT(list != NULL);
	
	list->id = id;
	list->state = SRC_STATE_IDEL;
	list->next = NULL;
	list->prev = NULL;
	list->src_name = name;
	
	if(*head){
		while(p->next){
			p = p->next;
		}
		
		p->next = list;
		list->prev = p;
	}else{
		*head = list;
	}
	
	/*SRC_LINE ahould always be the last constant source*/
	if(list->id == SRC_LINE){
		(*head)->prev = list;
	}
	
}


/*this function will called before mdc init, so the list will only contain constant sources*/
void sys_src_list_create(sys_state_t *sys, src_list_t **head)
{
  uint8 j;
	
	for(j = 0; j < SRC_CONSTANT_NUM; j++){
		src_list_add_next(head, j, constant_src_name[j]);
	}
	
}

/*detach all the source except for the constant source*/
void sys_src_list_detach(src_list_t **head)
{
	uint8 j = 0;
	src_list_t **cur;
	src_list_t *entry;
//	src_list_t *p;
//	
//	SYS_TRACE("before sys_src_list_detach:");
//	for(p = *head; p != NULL; p = p->next){
//		SYS_TRACE("%s->", p->src_name);
//	}
//	SYS_TRACE("\r\n");
	
	for(cur = head; *cur != NULL;){
		entry = *cur;
		if(j++ >= SRC_CONSTANT_NUM){
			*cur = entry->next;
			 FREE(entry);
		}else{
			cur = &entry->next;
		}
	}
	
//	SYS_TRACE("after sys_src_list_detach:");
//	for(p = *head; p != NULL; p = p->next){
//		SYS_TRACE("%s->", p->src_name);
//	}
//	SYS_TRACE("\r\n");
//	
}

void sys_src_load_cur(sys_state_t *sys, src_list_t **cur)
{
	src_list_t *p = gSrcListHead;
	
	while(p){
		if(p->id == sys->src){
			*cur = p;
			SYS_TRACE("sys_src_load_cur is:%s\r\n", p->src_name);
			break;
		}
		
		p = p->next;
	}
	
	if(p == NULL){
		SYS_TRACE("sys_src_load_cur fail!!! set to gSrcListHead\r\n");
		*cur = gSrcListHead;
	}
}

void sys_src_list_append(src_list_t *head, mdc_setup_t *s, mdc_slot_state_t *slot)
{
	src_list_t *p = head;
	uint8 j = 0;
	src_list_t *list;
	uint8 cnt;
	
	OSAL_ASSERT(p != NULL);
	
	/*if a DIO card used, we sholud modify the constant DIO source name*/
	if(s->type == MDC_DIO){
		while(p){
			if(p->id <= SRC_OPT2){
				p->src_name = constant_src_name_2[j++];
			}else{
				break;
			}
			p = p->next;
		}
	}
	
	/*get to the list end*/
	while(p->next){
		p = p->next;
	}
	
	if(slot->type == MDC_USB && slot->index > 0){
		cnt = slot->max_input - 1;
	}else{
		cnt = slot->max_input;
	}
	
	for(j = 0; j < cnt; j++){
		list = (src_list_t *)MALLOC(sizeof(src_list_t));
		OSAL_ASSERT(list != NULL);
		list->id = p->id + 1;
		list->state = SRC_STATE_IDEL;
		list->next = NULL;
		list->prev = p;
		list->src_name = (*slot->name)[j];
		
		p->next = list;
		p = p->next;
		
		SYS_TRACE("append %d.%s\r\n", list->id, list->src_name);
	}
	
	head->prev = p;
}

void sys_cur_src_state_update(src_state_t state, uint8 src_id)
{
	src_list_t *p = gSrcListHead;
	
	while(p){
		if(p->id == src_id){
			//p->state = state;
			break;
		}
		p = p->next;
	}
	/*it is very difficult to figure out this function*/
	if(strstr(p->src_name, "BluO")){
		p->state = state;
	}else if(strstr(p->src_name, "Computer")){
		gCurSrc->state = state;
	}
	
}

void sys_update_all_src_state(src_state_t state)
{
	src_list_t *p = gSrcListHead;
	
	while(p){
		p->state = state;
		p = p->next;
	}
	
}

void sys_update_src_state(src_state_t state, uint8 src_id)
{
	src_list_t *p = gSrcListHead;
	
	while(p){
		if(p->id == src_id){
			p->state = state;
			break;
		}
		p = p->next;
	}
	
}

void sys_update_src_state_by_name(src_state_t state, char *name)
{
	src_list_t *p = gSrcListHead;
	
	while(p){
		if(!strncmp(p->src_name, name, strlen(name))){
			p->state = state;
			break;
		}
		p = p->next;
	}
	
}

src_state_t sys_get_src_state(char *name)
{
	src_state_t state;
	
	src_list_t *p = gSrcListHead;
	
	while(p){
		if(!strncmp(p->src_name, name, strlen(name))){
			 state = p->state;
			break;
		}
		p = p->next;
	}
	
	
	return state;
}

void sys_usb_file_name_record(file_list_t **head, uint8 num, const char *name)
{
	file_list_t *list;
	file_list_t *p = *head;
	char *n;
	
	list = (file_list_t *)MALLOC(sizeof(file_list_t));
	
	OSAL_ASSERT(list != NULL);
	
	list->id = num;
	list->next = NULL;
	list->prev = NULL;
	n = (char *)MALLOC(strlen(name) + 1);
	memset(n, 0, strlen(name) + 1);
	strcpy(n, name);
	list->file_name = n;
	
	if(*head){
		/*append the list end*/
		while(p->next){
		 p = p->next;
		}
		
		p->next = list;
		list->prev = p;
		
	}else{
		*head = list;
	}
	
	if((num + 1) == gcurrent_usb_file_cnt ){
		(*head)->prev = list;
	}
	
}

void sys_file_list_load_cur(file_list_t *head, file_list_t **cur)
{
	*cur = head;
}

static void do_delect_file_list(file_list_t **head, file_list_t **cur)
{
	file_list_t *p = *head;
	file_list_t *tail;
	
	*cur = NULL;
	
	if(p == NULL) return;
	
	  while(p->next){
				p = p->next;
			}
			
		/*it is important to cut off the head prev point*/
		(*head)->prev = NULL;
	
		while(*head){
			
			tail = p;
		
			if(p->prev){
				p = p->prev;
			}else{
				/*delect the head*/
				*head = NULL;
			}
		
		  FREE(tail->file_name);
			tail->file_name = NULL;
			FREE(tail);
			tail = NULL;
			if(p){
			  p->next = NULL;
			}
			
		}
	
}

static void sys_usb_file_next(mdc_slot_t slot)
{
	uint8 cnt;
	
	if(gCurFile[slot]){
		if(gCurFile[slot]->next){
			gCurFile[slot] = gCurFile[slot]->next;
		}else{
			if(gcurrent_usb_file_cnt > (gCurrent_usb_record_file_cnt + 1)){
				cnt = gcurrent_usb_file_cnt - gCurrent_usb_record_file_cnt - 1;
				mdc_usb_send_msg(slot, "Media.FnRequest=%d,%d\n",gCurrent_usb_record_file_cnt + 1, (cnt > 8? 8:cnt));
				Send_DisplayTask_Msg(DIS_JUMP, USB_READING_MENU);
				osal_start_timerEx(gSysTaskID, USB_FILE_LIST_UPDATE_MSG,  800);
				return;
			}else{
				gCurFile[slot] = gFileListHead[slot];
			}
		}
	}else{
		gCurFile[slot] = gFileListHead[slot];
	}
	
	Send_DisplayTask_Msg(DIS_UPDATE, 0);
}

static void sys_usb_file_prev(mdc_slot_t slot)
{
	if(gCurFile[slot]){
		if(gCurFile[slot]->prev){
			gCurFile[slot] = gCurFile[slot]->prev;
		}
		
	}else{
		gCurFile[slot] = gFileListHead[slot];
	}
	
	
}

static void do_file_seek(sys_state_t *sys, char *msg, mdc_slot_t slot)
{
	int cnt = 0;
	int retval = 0;
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	retval = sscanf(msg, "UI.FileCount=%d", &cnt);
	if(retval == 1){
		if(cnt){
			mdc_usb_send_msg(slot, "Media.FnRequest=0,%d\n", (cnt > 8 ? 8:cnt));
		}else{
			/*there is no support file found*/
		}
	}
	
	gcurrent_usb_file_cnt = cnt;
	//do_delect_file_list(&gFileListHead[slot], &gCurFile[slot]);
	if(strstr(gCurSrc->src_name, "USB")){
		display_menu_jump(USB_FILE_LIST_MENU);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		Deregister_Bitmap_Flag(USB_FILE_LIST_SHOW_LOCK);
	}
}

static void do_music_play_show(sys_state_t *sys, char *msg, mdc_slot_t slot)
{
	int retval;
	mdc_t *p;
	int len = strlen(msg) - strlen("UI.Title=\n");
	
	p = (mdc_t *)sys->mdc;
	
	
	if(len <= 0) return;
	
	if(gCurTitle[slot]){
		FREE(gCurTitle[slot]);
		gCurTitle[slot] = NULL;
	}
	
	gCurTitle[slot] = (char *)MALLOC(len);
	
	memset(gCurTitle[slot], 0, len);
		
	retval = sscanf(msg, "UI.Title=%[^\n]", gCurTitle[slot]);
	
	sys_cur_src_state_update(SRC_STATE_PLAY, p->slot_src_idx[slot]);
	gPlayTime = 0;
	if(gCurSrc->id >= SRC_CONSTANT_NUM && p->mdc_slot_index == slot){
		display_menu_jump(USB_PLAY_MENU);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}else{
		dis_scroll_update(gCurTitle[slot]);
	}
}

static void do_record_file_name(char *msg, mdc_slot_t slot)
{
	int cnt = 0;
	int retval = 0;
	char name[64] = {0};

	
	retval = sscanf(msg, "UI.FileName=%d,%[^\n]", &cnt, name);
  
	if(retval == 2){
		sys_usb_file_name_record(&gFileListHead[slot], cnt, name);
	}
	
	/*when we record all the file name we can let the dispaly task show that*/
	if(gcurrent_usb_file_cnt == (cnt + 1) || cnt >= 4){
		if(false == IS_Flag_Valid(USB_FILE_LIST_SHOW_LOCK)){
			sys_file_list_load_cur(gFileListHead[slot], &gCurFile[slot]);
			display_menu_jump(USB_FILE_LIST_MENU);
			Send_DisplayTask_Msg(DIS_UPDATE, 0);
			Register_Bitmap_Flag(USB_FILE_LIST_SHOW_LOCK);
		}
		
		if(gcurrent_usb_file_cnt == (cnt + 1)){
			Deregister_Bitmap_Flag(USB_FILE_LIST_SHOW_LOCK);
		}
	}
	
	sys_update_src_state(SRC_STATE_FILE_LIST, gCurSrc->id);
	gCurrent_usb_record_file_cnt = cnt;
}

static void do_music_play_time_update(sys_state_t *sys, char *msg, mdc_slot_t slot)
{
	int time;
	int retval;
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	/*we need discard the other slot infor when the 2 slot paly the music at the same time*/
	if(p->mdc_slot_index != slot) return;
	
	retval = sscanf(msg, "UI.PlayTime=%d", &time);
	if(retval == 1){
		gPlayTime = time;
		if(gCurSrc->state == SRC_STATE_PLAY && cur_menu->id == USB_PLAY_MENU){
			Send_DisplayTask_Msg(DIS_UPDATE, 0);
		}
	}

}

static void do_music_play_control(sys_state_t *sys, char *msg, mdc_slot_t slot)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(strstr(msg, "Pause")){
		sys_cur_src_state_update(SRC_STATE_PAUSE, p->slot_src_idx[slot]);
		Send_DisplayTask_Msg(DIS_UPDATE, 1);
	}else if(strstr(msg, "Play")){
		sys_cur_src_state_update(SRC_STATE_PLAY, p->slot_src_idx[slot]);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}
	
}

static void do_usb_plug_out_hanlder(sys_state_t *sys, mdc_slot_t slot)
{
	do_delect_file_list(&gFileListHead[slot], &gCurFile[slot]);
	
	if(strstr(gCurSrc->src_name, "USB")){
		display_menu_jump(USB_NOT_INSERT_MENU);
	  Send_DisplayTask_Msg(DIS_UPDATE, 0);
		if(!strncmp(gCurSrc->src_name, "USB Local", strlen("USB Local"))){
			sys_update_src_state_by_name(SRC_STATE_NO_DEVICE, "USB Back");
		}else{
			sys_update_src_state_by_name(SRC_STATE_NO_DEVICE, "USB Local");
		}
		
	}else{
		/*handler for when pulg out usb flash driver in other source*/
		sys_update_src_state_by_name(SRC_STATE_NO_DEVICE, "USB Local");
		sys_update_src_state_by_name(SRC_STATE_NO_DEVICE, "USB Back");
	}
	
	
}

void sys_mdc_usb_msg_handler(mdc_slot_t slot, char *msg)
{
	if(!strncmp(msg, "UI.FileCount=", strlen("UI.FileCount="))){
		do_file_seek(gSystem_t ,msg, slot);
	}else if(!strncmp(msg, "UI.FileName=", strlen("UI.FileName="))){
		do_record_file_name(msg, slot);
	}else if(!strncmp(msg, "UI.DirLevel=", strlen("UI.DirLevel="))){
		/*when seek into next dir we should delect current filelist */
		do_delect_file_list(&gFileListHead[slot], &gCurFile[slot]);
	}else if(!strncmp(msg, "UI.Title=", strlen("UI.Title="))){
		do_music_play_show(gSystem_t,msg, slot);
	}else if(!strncmp(msg, "UI.PlayTime=", strlen("UI.PlayTime="))){
		do_music_play_time_update(gSystem_t ,msg, slot);
	}else if(!strncmp(msg, "UI.DiskState=Removed", strlen("UI.DiskState=Removed"))){
		do_usb_plug_out_hanlder(gSystem_t, slot);
	}else if(!strncmp(msg, "UI.PlayState=", strlen("UI.PlayState="))){
		do_music_play_control(gSystem_t ,&msg[strlen("UI.PlayState=")], slot);
	}
}

static void do_blu_mute_reply(sys_state_t *sys, mdc_slot_t slot)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	blu_send_msg(slot, "Main.Mute=off\n");
	
}

static void do_blu_volume_reply(sys_state_t *sys, char *msg, mdc_slot_t slot)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(strchr(msg, '=')){
		sscanf(msg, "Main.Volume=%f\n", &sys->master_vol);
		sys_master_vol_set(sys);	
	  Send_DisplayTask_Msg(DIS_JUMP, SOURCE_MENU);
	  Register_Bitmap_Flag(DISPLAY_UPDATE_LOCK);
	  osal_start_timerEx(gDisTaskID, VOL_DIS_BACK_MSG,  VOL_MENU_DIS_TIMEOUT);
	}
	SYS_TRACE("master volume=%.1f\r\n", sys->master_vol);
	blu_send_msg(slot, "Main.Volume=%.1f\n", sys->master_vol);
	
}

static void do_blu_status_update(sys_state_t *sys, char *msg, mdc_slot_t slot)
{
	int retval;
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	if(strstr(msg, "Connecting")){
		sys->mbs[slot] = CONNECTING_TO_NETWORK;
	}else if(strstr(msg, "Hotspot")){
		sys->mbs[slot] = HOTSPOT_MODE;
	}
	
	if(strstr(gCurSrc->src_name, "BluO")){
		if(p->mdc_slot_index == slot){
			display_menu_jump(BLU_STATUS_MENU);
			Send_DisplayTask_Msg(DIS_UPDATE, sys->mbs[slot]);
		}
	}
}


void sys_mdc_blu_msg_handler(mdc_slot_t slot, char *msg)
{
	if(!strncmp(msg, "Main.Mute", strlen("Main.Mute"))){
		do_blu_mute_reply(gSystem_t, slot);
	}else if(!strncmp(msg, "Main.Volume", strlen("Main.Volume"))){
		do_blu_volume_reply(gSystem_t, msg, slot);
	}else if(!strncmp(msg, "UI.Status=", strlen("UI.Status="))){
		do_blu_status_update(gSystem_t, msg, slot);
	}else if(!strncmp(msg, "UI.Title=", strlen("UI.Title="))){
		do_music_play_show(gSystem_t ,msg, slot);
	}else if(!strncmp(msg, "UI.PlayTime=", strlen("UI.PlayTime="))){
		do_music_play_time_update(gSystem_t ,msg, slot);
	}else if(!strncmp(msg, "UI.PlayState=", strlen("UI.PlayState="))){
		do_music_play_control(gSystem_t ,&msg[strlen("UI.PlayState=")], slot);
	}
	
}

static void do_auto_standby_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	switch(symbol){
		case '?':
			
			break;
		
		case '=':
			if(!strcasecmp(argv[1], "Off")){
				SYS_TRACE("set auto to off\r\n");
				sys->is_auto_standby = false;
			}else{
				SYS_TRACE("set auto to on\r\n");
				sys->is_auto_standby = true;
			}
			
			Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
			
		case '+':
			SYS_TRACE("set auto +\r\n");
		  sys->is_auto_standby = (sys->is_auto_standby == false ? true : false);
		  sys_auto_stanby_onoff_handler(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case '-':
			SYS_TRACE("set auto -\r\n");
		  sys->is_auto_standby = (sys->is_auto_standby == false ? true : false);
		  sys_auto_stanby_onoff_handler(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
	}
	
	nad_send_str("\rMain.AutoStandby=%s\r", (sys->is_auto_standby == true ? ("On"):("Off")));
}

static void do_ir_channel_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	uint8 ch = 0;
	
	switch(symbol){
		case '?':
			
			break;
		
		case '=':
			ch = atoi(argv[1]);
		  if(ch >= 0 && ch <= 3){
				sys->ir_channel = ch;
				Send_DisplayTask_Msg(DIS_UPDATE, 0);
			}
			break;
		
		case '+':
			sys->ir_channel = (sys->ir_channel == 3 ? 0:(sys->ir_channel + 1));
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
		case '-':
			sys->ir_channel = (sys->ir_channel == 0 ? 3:(sys->ir_channel - 1));
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
			break;
		
	}
	
	
	nad_send_str("\rMain.IR.Channel=%d\r", sys->ir_channel);
}

static void do_model_msg_handler(void)
{
#ifdef NAD_C358
	nad_send_str("\rMain.Model=C358\r");
#else
	nad_send_str("\rMain.Model=C378\r");
#endif
	
}

static void do_mute_mag_handler(sys_state_t *sys, char symbol, char *argv[])
{
	switch(symbol){
		case '=':
			if(!strcasecmp(argv[1], "Off")){
				sys->is_mute = false;
			}else{
				sys->is_mute = true;
			}
			break;
			
		case '+':
		case '-':
			sys->is_mute = (sys->is_mute == true? false:true);
			break;
		
	}
	
	nad_send_str("\rMain.Mute=%s\r", (sys->is_mute == false ? ("Off"):("On")));
}

static void do_power_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	switch(symbol){
		case '=':
			if(!strcasecmp(argv[1], "Off")){
				if(sys->status != STATUS_STANDBY){
					sys_power_off_handler(sys);
				}
			}else{
				if(sys->status == STATUS_STANDBY){
					sys_power_on_handler(sys);
				}
			}
			break;
			
		case '+':
		case '-':
			if(sys->status == STATUS_STANDBY){
					sys_power_on_handler(sys);
				}else{
					sys_power_off_handler(sys);
				}
			break;
	}
	
	nad_send_str("\rMain.Power=%s\r", (sys->status == STATUS_STANDBY ? ("Off"):("On")));
}

static void do_source_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	uint8 src = 0;
	
	
	switch(symbol){
		case '=':
			src = atoi(argv[1]);
		  if(src >= 1 && src <= sys->src_num){
				sys->src = src - 1;
				sys_input_select(sys);
				display_menu_jump(SOURCE_MENU);
				Send_DisplayTask_Msg(DIS_UPDATE, 0);
				osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
			}
			break;
		
		case '+':
			sys_src_next_handler(sys);
			break;
		
		case '-':
			sys_src_prev_handler(sys);
			break;
		
	}
	
	nad_send_str("\rMain.Source=%d\r", sys->src + 1);
	
}

static void do_source_num_msg_handler(sys_state_t *sys)
{
	nad_send_str("\rMain.Sources=%d\r", sys->src_num);
}


static void do_version_msg_handler(void)
{
	nad_send_str("\rMain.Version=%s\r", version);
}


static void do_volume_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	
	float vol;
	
	if(symbol == '='){
		vol = atof(argv[1]);
		if(vol >= MASTER_VOL_INDEX_MIN && vol <= MASTER_VOL_INDEX_MAX){
			sys->master_vol = vol;
			sys_vol_control_handler(sys, DIR_EQU, 0);
		}
	}else if(symbol == '+'){
		sys_vol_control_handler(sys, DIR_UP, 0);
	}else if(symbol == '-'){
		sys_vol_control_handler(sys, DIR_DOWN, 0);
	}
	
  nad_send_str("\rMain.Volume=%.1f\r", sys->master_vol);
}

static void do_action_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	if(!strcasecmp(argv[1], "FactoryReset")){
		nad_send_str("\rMain.Action=FactoryReset\r");
		
		sys_factory_reset(sys);
	}
	
}

void do_main_msg_handler(sys_state_t *sys)
{
	
#ifdef NAD_C358
	nad_send_str("\rMain.Model=C358\r");
#else
	nad_send_str("\rMain.Model=C378\r");
#endif
	nad_send_str("\rMain.Mute=%s\r", (sys->is_mute == false ? ("Off"):("On")));
	nad_send_str("\rMain.Power=%s\r", (sys->status == STATUS_STANDBY ? ("Off"):("On")));
	nad_send_str("\rMain.Source=%d\r", sys->src);
	nad_send_str("\rMain.Sources=%d\r", sys->src_num);
	nad_send_str("\rMain.Version=%s\r", version);
	nad_send_str("\rMain.Volume=%.1f\r", sys->master_vol);
	nad_send_str("\rMain.AutoStandby=%s\r", (sys->is_auto_standby == true ? ("On"):("Off")));
	nad_send_str("\rMain.IR.Channel=%d\r", sys->ir_channel);
	if(sys->balance_l < 0){
		nad_send_str("\rMain.Balance=%dL\r", sys->balance_l);
	}else{
		nad_send_str("\rMain.Balance=%dR\r", sys->balance_r);
	}
	nad_send_str("\rMain.Bass=%d\r", sys->bass_vol);
	nad_send_str("\rMain.Treble=%d\r", sys->treble_vol);
	nad_send_str("\rMain.SpeakerA=%s\r", (sys->speaker_mode <= SPEAKER_A) ? "On" : "Off");
	nad_send_str("\rMain.SpeakerB=%s\r", ((sys->speaker_mode == SPEAKER_B) || (sys->speaker_mode == SPEAKER_A_B)) ? "On" : "Off");
}

static void do_balance_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	
	int value = 0;
	
	if(symbol == '='){
		value = atoi(argv[1]);
		SYS_TRACE("do_balance_msg_handler:v=%d\r\n", value);
		
		if(strchr(argv[1], 'l') || strchr(argv[1], 'L')){
			if(value >= SYS_BALANCE_MIN && value <= 0){
				sys->balance_l = value;
				sys->balance_r = 0;
			}
		}else if(strchr(argv[1], 'R') || strchr(argv[1], 'r')){
			if(value >= SYS_BALANCE_MIN && value <= 0){
				sys->balance_r = value;
				sys->balance_l = 0;
			}
		}
		
		sys_balance_set(sys);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
	}else if(symbol == '+'){
		if(sys->balance_l < 0){
				 sys->balance_l = (sys->balance_l == 0) ? 0 : (sys->balance_l + 1);
			 }else{
				 sys->balance_r = (sys->balance_r == SYS_BALANCE_MIN) ? SYS_BALANCE_MIN : (sys->balance_r - 1);
			 }
		
			
			sys_balance_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}else if(symbol == '-'){
		if(sys->balance_r < 0){
				sys->balance_r = (sys->balance_r == 0) ? 0 : (sys->balance_r + 1);
			}else{
				sys->balance_l = (sys->balance_l == SYS_BALANCE_MIN) ? SYS_BALANCE_MIN : (sys->balance_l - 1);
			}
				
			sys_balance_set(sys);
		  Send_DisplayTask_Msg(DIS_UPDATE, 0);
		
	}
	
	if(sys->balance_l < 0){
		nad_send_str("\rMain.Balance=%dL\r", sys->balance_l);
	}else{
		nad_send_str("\rMain.Balance=%dR\r", sys->balance_r);
	}
	
}

static void do_bass_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	int bass;
	
	if(symbol == '='){
		bass = atoi(argv[1]);
		if(bass <= SYS_BASS_VOL_MAX && bass >= SYS_BASS_VOL_MIN){
			sys->bass_vol = bass;
			sys_bass_vol_set(sys);
			Send_DisplayTask_Msg(DIS_UPDATE, 0);
			osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		}
	}else if(symbol == '+'){
		sys->bass_vol = (sys->bass_vol == SYS_BASS_VOL_MAX) ? SYS_BASS_VOL_MAX : (sys->bass_vol + 1);
		sys_bass_vol_set(sys);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
	}else if(symbol == '-'){
		sys->bass_vol = (sys->bass_vol == SYS_BASS_VOL_MIN) ? SYS_BASS_VOL_MIN : (sys->bass_vol - 1);
		sys_bass_vol_set(sys);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
	}
	
	nad_send_str("\rMain.Bass=%d\r", sys->bass_vol);
}


static void do_treble_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	int treble;
	
	if(symbol == '='){
		treble = atoi(argv[1]);
		if(treble <= SYS_TREBLE_VOL_MAX && treble >= SYS_TREBLE_VOL_MIN){
			sys->treble_vol = treble;
			sys_treble_vol_set(sys);
			Send_DisplayTask_Msg(DIS_UPDATE, 0);
			osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		}
	}else if(symbol == '+'){
		sys->treble_vol = (sys->treble_vol == SYS_TREBLE_VOL_MAX) ? SYS_TREBLE_VOL_MAX : (sys->treble_vol + 1);
		sys_treble_vol_set(sys);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
	}else if(symbol == '-'){
		sys->treble_vol = (sys->treble_vol == SYS_TREBLE_VOL_MIN) ? SYS_TREBLE_VOL_MIN : (sys->treble_vol - 1);
		sys_treble_vol_set(sys);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
	}
	
	nad_send_str("\rMain.Treble=%d\r", sys->treble_vol);
}



static void do_speaker_a_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	if(symbol == '='){
		if(!strcasecmp(argv[1], "On")){
			if(sys->speaker_mode == SPEAKER_B){
				sys->speaker_mode = SPEAKER_A_B;
			}else if(sys->speaker_mode == SPEAKER_OFF){
				sys->speaker_mode = SPEAKER_A;
			}
		}else if(!strcasecmp(argv[1], "Off")){
			if(sys->speaker_mode == SPEAKER_A){
				sys->speaker_mode = SPEAKER_OFF;
			}else if(sys->speaker_mode == SPEAKER_A_B){
				sys->speaker_mode = SPEAKER_B;
			}
		}
		
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		sys_speaker_mode_select_handler(sys);
	}else if(symbol == '+' || symbol == '-'){
		if(sys->speaker_mode == SPEAKER_A_B){
			sys->speaker_mode = SPEAKER_B;
		}else if(sys->speaker_mode == SPEAKER_A){
			sys->speaker_mode = SPEAKER_OFF;
		}else if(sys->speaker_mode == SPEAKER_B){
			sys->speaker_mode = SPEAKER_A_B;
		}else if(sys->speaker_mode == SPEAKER_OFF){
			sys->speaker_mode = SPEAKER_A;
		}
		
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		sys_speaker_mode_select_handler(sys);
	}
	
	nad_send_str("\rMain.SpeakerA=%s\r", (sys->speaker_mode <= SPEAKER_A) ? "On" : "Off");
	
}


static void do_speaker_b_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	if(symbol == '='){
		if(!strcasecmp(argv[1], "On")){
			if(sys->speaker_mode == SPEAKER_A){
				sys->speaker_mode = SPEAKER_A_B;
			}else if(sys->speaker_mode == SPEAKER_OFF){
				sys->speaker_mode = SPEAKER_B;
			}
		}else if(!strcasecmp(argv[1], "Off")){
			if(sys->speaker_mode == SPEAKER_B){
				sys->speaker_mode = SPEAKER_OFF;
			}else if(sys->speaker_mode == SPEAKER_A_B){
				sys->speaker_mode = SPEAKER_A;
			}
		}
		
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		sys_speaker_mode_select_handler(sys);
	}else if(symbol == '+' || symbol == '-'){
		if(sys->speaker_mode == SPEAKER_A_B){
			sys->speaker_mode = SPEAKER_A;
		}else if(sys->speaker_mode == SPEAKER_B){
			sys->speaker_mode = SPEAKER_OFF;
		}else if(sys->speaker_mode == SPEAKER_A){
			sys->speaker_mode = SPEAKER_A_B;
		}else if(sys->speaker_mode == SPEAKER_OFF){
			sys->speaker_mode = SPEAKER_B;
		}
		
		osal_start_timerEx(gSysTaskID, DATABASE_SAVE_MSG, 300);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
		sys_speaker_mode_select_handler(sys);
	}
	
	nad_send_str("\rMain.SpeakerB=%s\r", ((sys->speaker_mode == SPEAKER_B) || (sys->speaker_mode == SPEAKER_A_B)) ? "On" : "Off");
	
}


static void do_tone_defeat_msg_handler(sys_state_t *sys, char symbol, char *argv[])
{
	
	
	
	
}

static char parse_line(char *line, char *argv[])
{
	uint8 argc = 0;
	char  symbol = 0;
	
	argv[argc++] = line;
	
	while(*line != '=' && *line != '?' && *line != '+' && *line != '-' && *line != '\0'){
		line++;
	}
	
	if(*line == '\0'){
		//SYS_TRACE("get invalid nad msg\r\n");
		argv[argc] = NULL;
	}else if(*line == '='){
		*line++ = '\0';
		argv[argc++] = line;
		symbol = '=';
	}else{
		symbol = *line;
		*line = '\0';
	}
	
	return symbol;
}

#define BLE_DATA_LENGTH_MAX 20

static void ble_cmd_handler(char *msg)
{
	uint32 data[BLE_DATA_LENGTH_MAX] = {0};
  char *p;
  uint8 j=0;
  uint8 i;

  if(msg == NULL) return;

  p = strtok(msg, " ");

  while(p){
		if(j < BLE_DATA_LENGTH_MAX){
			sscanf(p, "%x", &data[j]);
		}
		p = strtok(NULL, " ");
		j++;
	}
	
  if(j > BLE_DATA_LENGTH_MAX) j = BLE_DATA_LENGTH_MAX;
	
	bt_send_byte(0x50);
	bt_send_byte(j);
	for(i = 0; i < j; i++){
		bt_send_byte((uint8)data[i]);
	}
	bt_send_byte(0x55);
	bt_send_byte(0xff);

}


extern bool nad_app_echo;

void sys_nad_uart_msg_handler(sys_state_t *sys, char *msg)
{
	char *argv[3] = {NULL};
  char symbol;

 // SYS_TRACE("nad msg:%s\r\n", msg);

  if(!strncasecmp(msg, "ble", strlen("ble"))){
		ble_cmd_handler(&msg[4]);
	}

	symbol = parse_line(msg, argv);
	
	if(!strcasecmp(argv[0], "Main.AutoStandby")){
		do_auto_standby_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.IR.Channel")){
		do_ir_channel_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.Model")){
		do_model_msg_handler();
	}else if(!strcasecmp(argv[0], "Main.Mute")){
		do_mute_mag_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.Power")){
		do_power_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.Source")){
		do_source_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.Sources")){
		do_source_num_msg_handler(sys);
	}else if(!strcasecmp(argv[0], "Main.Version")){
		do_version_msg_handler();
	}else if(!strcasecmp(argv[0], "Main.Volume")){
		do_volume_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main")){
		do_main_msg_handler(sys);
	}else if(!strcasecmp(argv[0], "Main.Action")){
		do_action_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.Balance")){
		do_balance_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.Bass")){
		do_bass_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.Treble")){
		do_treble_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.SpeakerA")){
		do_speaker_a_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.SpeakerB")){
		do_speaker_b_msg_handler(sys, symbol, argv);
	}else if(!strcasecmp(argv[0], "Main.ToneDefeat")){
		do_tone_defeat_msg_handler(sys, symbol, argv);
	}
	
	
	if(!strcmp(argv[0], "echo")){
		nad_app_echo = true;
	}else if(!strcmp(argv[0], "log")){
		log_on_nad = true;
	}
	
}

void sys_audio_valid_handler(uint8 index)
{
	SYS_TRACE("sys_audio_valid\r\n");
	
	sys_update_src_state(SRC_STATE_PLAY, SRC_MM);
	sys_update_src_state(SRC_STATE_PLAY, SRC_LINE);
}

void sys_audio_invalid_handler(uint8 index)
{
	SYS_TRACE("sys_audio_invalid\r\n");
	
	if(gSystem_t->status != STATUS_WORKING) return;
	
	sys_update_src_state(SRC_STATE_IDEL, SRC_MM);
	sys_update_src_state(SRC_STATE_IDEL, SRC_LINE);
}

void sys_trigger_plug_in_handler(uint8 index)
{
	SYS_TRACE("sys_trigger_plug_in\r\n");
	update_detect_state(DETECT_TRIGGER_LEVEL);
	
}

void sys_trigger_plug_out_handler(uint8 index)
{
	SYS_TRACE("sys_trigger_plug_out\r\n");

	
}

void sys_protection_valid_handler(uint8 index)
{
	
	/*since more than one protection event may happen at the same time, we should record these*/
	gSystem_t->protect_bitmap |= (1 << index);
	
	SYS_TRACE("sys protection valid:%d, protect_bitmap=%2x\r\n", index, gSystem_t->protect_bitmap);
	
	if(gSystem_t->status != STATUS_WORKING && gSystem_t->status != STAUS_INITING){
		/*when protection event happen in standby or shutdown mode, we need do noting but just record the event*/
		return;
	}
	
	if(index == DETECT_AC_DET){
		Send_DisplayTask_Msg(DIS_JUMP, AC_ERROR_MENU);	
		AMP_MUTE_ON();
	  AMP_MUTE2_ON();
	}else if(index == DETECT_DC_ERROR
#ifdef NAD_C378
	|| index ==	DETECT_DC2_ERROR
#endif
	){
		
		AMP_MUTE_ON();
	  AMP_MUTE2_ON();
		SPEAKER1_MUTE_ON();
		SPEAKER2_MUTE_ON();
		Send_DisplayTask_Msg(DIS_JUMP, DC_ERROR_MENU);	
	}else if(index == DETECT_OVER_TEMP_R
#ifdef NAD_C378
	|| index ==	DETECT_OVER_TEMP_L
#endif
	){
		
#ifdef NAD_C378
	if(IS_OVER_TEMP_L && IS_OVER_TEMP_R){
		AMP_MUTE_ON();
	  AMP_MUTE2_ON();
	}else if(IS_OVER_TEMP_L){
		AMP_MUTE_ON();
	}else{
		AMP_MUTE2_ON();
	}
#else
	  AMP_MUTE_ON();
	  AMP_MUTE2_ON();
	  
	  SPEAKER1_MUTE_ON();
		SPEAKER2_MUTE_ON();
#endif
		Send_DisplayTask_Msg(DIS_JUMP, OVER_TEMP_MENU);	
	}
	
	ui_led_control(LED_RED);
	
//	/*when protection valid we will delay sometime to check invalid*/
//	set_detect_state(index, DETECT_STOP);
//  osal_start_timerEx(gNADUartID, PROTECTION_LATER_MSG, 5000);	
	
}

void sys_protection_invalid_handler(uint8 index)
{
	
	if(gSystem_t->protect_bitmap == 0){
		/*since no protection event happened, we need do nothing*/
		return;
	}else{
		gSystem_t->protect_bitmap &= ~(1 << index);
	}
	
	SYS_TRACE("sys protection invalid:%d, protect_bitmap=%2x\r\n", index, gSystem_t->protect_bitmap);
	
	if(gSystem_t->status != STATUS_WORKING && gSystem_t->status != STAUS_INITING) return;
	
	if(gSystem_t->protect_bitmap == 0){
		AMP_MUTE_OFF();
	  AMP_MUTE2_OFF();
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}
#ifdef NAD_C378
	else if(gSystem_t->protect_bitmap == (1 << DETECT_OVER_TEMP_R)){
		/*only right temp protction, so update the dispaly and anbale the left amp*/
		Send_DisplayTask_Msg(DIS_JUMP, OVER_TEMP_MENU);	
		AMP_MUTE_OFF();
	}else if(gSystem_t->protect_bitmap == (1 << DETECT_OVER_TEMP_L)){
		/*only left temp protction, so update the dispaly and anbale the right amp*/
		Send_DisplayTask_Msg(DIS_JUMP, OVER_TEMP_MENU);	
		AMP_MUTE2_OFF();
	}
#endif
}

void sys_trigger_valid_handler(uint8 index)
{
	SYS_TRACE("sys_trigger_valid\r\n");
  if(IS_TRIGGER_IN && gSystem_t->status == STATUS_STANDBY){
		sys_power_on_handler(gSystem_t);
	}
}

void sys_trigger_invalid_handler(uint8 index)
{
	SYS_TRACE("sys_trigger_invalid\r\n");
	if(IS_TRIGGER_IN && gSystem_t->status != STATUS_STANDBY){
		sys_power_off_handler(gSystem_t);
	}
}

void sys_digital_audio_valid_handler(uint8 index)
{
	SYS_TRACE("sys_digital_audio_valid\r\n");
	
	sys_update_src_state(SRC_STATE_PLAY, SRC_COAX1);
	sys_update_src_state(SRC_STATE_PLAY, SRC_COAX2);
	sys_update_src_state(SRC_STATE_PLAY, SRC_OPT1);
	sys_update_src_state(SRC_STATE_PLAY, SRC_OPT2);
	
	/*for MDC Test Card*/
	if(gSystem_t->mdc_test){
		Send_Soft_Task_Msg(SOFT_USER_MSG, 1);
	}
	
}

void sys_digital_audio_invalid_handler(uint8 index)
{
	SYS_TRACE("sys_digital_audio_invalid\r\n");
	if(gSystem_t->status != STATUS_WORKING) return;
	
	sys_update_src_state(SRC_STATE_IDEL, SRC_COAX1);
	sys_update_src_state(SRC_STATE_IDEL, SRC_COAX2);
	sys_update_src_state(SRC_STATE_IDEL, SRC_OPT1);
	sys_update_src_state(SRC_STATE_IDEL, SRC_OPT2);
}

void sys_hp_plug_in_handler(uint8 index)
{
	SYS_TRACE("sys_hp_plug_in\r\n");
	
  SPEAKER1_MUTE_ON();	
	SPEAKER2_MUTE_ON();

	AMP_MUTE_ON();
	AMP_MUTE2_ON();
	
	LINE_MUTE_ON();
	
	
	if(gSystem_t->status == STATUS_WORKING){
		HP_MUTE_OFF();
	}
	
}

void sys_hp_plug_out_handler(uint8 index)
{
	SYS_TRACE("sys_hp_plug_out\r\n");
	
	if(gSystem_t->status != STATUS_WORKING) return;
	
	HP_MUTE_ON();
	
	if(gSystem_t->speaker_mode == SPEAKER_A){
		SPEAKER1_MUTE_OFF();	
	}else if(gSystem_t->speaker_mode == SPEAKER_B){
		SPEAKER2_MUTE_OFF();
	}else if(gSystem_t->speaker_mode == SPEAKER_A_B){
		SPEAKER1_MUTE_OFF();	
		SPEAKER2_MUTE_OFF();
	}

	LINE_MUTE_OFF();
	
	AMP_MUTE_OFF();
	AMP_MUTE2_OFF();
}

/*********************************************************************/
int print_src(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	src_list_t *p = gSrcListHead;
	uint8 j = 0;
	
	while(p){
		
	 SYS_TRACE("%d.%s\r\n", j++, p->src_name);
		
	 p = p->next;
	}
	
	return 0;
}

OSAL_CMD(psrc, 1, print_src, "print src list ");

int src_next(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	
	if(gCurSrc){
		if(gCurSrc->next){
			gCurSrc = gCurSrc->next;
		}else{
			gCurSrc = gSrcListHead;
		}
		SYS_TRACE("get next src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}else{
		gCurSrc = gSrcListHead;
		SYS_TRACE("get next src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}
	
	return 0;
}

OSAL_CMD(srcn, 1, src_next, "src list next");


int src_prev(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	
	if(gCurSrc){
		OSAL_ASSERT(gCurSrc->prev);
		gCurSrc = gCurSrc->prev;
		SYS_TRACE("get next src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}else{
		gCurSrc = gSrcListHead;
		SYS_TRACE("get next src->%d.%s\r\n", gCurSrc->id, gCurSrc->src_name);
	}
	
	return 0;
}

OSAL_CMD(srcp, 1, src_prev, "src list next");


int power_on(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	  
	sys_power_on_handler(gSystem_t);
	return 0;
}

OSAL_CMD(po, 1, power_on, "power on");

extern void display_init_menu(void);

int lcd_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
		
  display_init_menu();
	
	SYS_TRACE("\r\n");
		
	return 0;
}

OSAL_CMD(lcd, 1, lcd_test, "lcd init test");

int pin_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	  
	if(strcmp(argv[1], "H") == 0){
		AMP_MUTE_ON();
	}else{
		AMP_MUTE_OFF();
	}
	return 0;
}

OSAL_CMD(pin, 2, pin_test, "lcd init test");

int hc595_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	int port, val;

	port = atoi(argv[1]);
  val = atoi(argv[2]);

  SYS_TRACE("hc595 write port:%d, val:%d\r\n", port, val);

  hc595_write(port, val);    
	
	return 0;
}

OSAL_CMD(hc595, 3, hc595_test, "lcd init test");


int hc595_init_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
//	int port, val;

//	port = atoi(argv[1]);
//  val = atoi(argv[2]);

//  SYS_TRACE("hc595 write port:%d, val:%d\r\n", port, val);

//  hc595_write(port, val);    
	
	hc595_init();
	
	SYS_TRACE("hc595_init_test\r\n");
	
	return 0;
}

OSAL_CMD(hc595_init, 1, hc595_init_test, "lcd init test");

int srcrst_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	SRC4382_RST_L;
	bsp_delay_ms(50);
	SRC4382_RST_H;
	bsp_delay_ms(50);
	
	SYS_TRACE("srcrst_test done\r\n");
	return 0;
}

OSAL_CMD(srcRst, 1, srcrst_test, "srcrst test");

int mdc_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  char buf[64] = {0};
	
  if(strcmp(argv[1], "Play") == 0){
		sys_music_play_handler(gSystem_t);
	}else{
		sys_music_pause_handler(gSystem_t);
	}
  //sys_music_pause_handler(gSystem_t);
	//mdc_usb_send_msg(SLOT_1, buf, "%s\n", argv[1]);
	SYS_TRACE("mdc test done\r\n");
	return 0;
}

OSAL_CMD(mdc, 2, mdc_test, "mdc test");

int bt_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  
	bt_send_byte(0x03);
	bt_send_byte(0x00);
	bt_send_byte(0x55);
	bt_send_byte(0xff);
		
	SYS_TRACE("bt test done\r\n");
	return 0;
}

OSAL_CMD(bt, 1, bt_test, "bt test");


int file_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  
//  sys_usb_file_next();
		
	SYS_TRACE("file test done\r\n");
	return 0;
}

OSAL_CMD(filen, 1, file_test, "bt test");

int factory_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  
  sys_nv_set_default(gSystem_t, atoi(argv[1]));
		
	SYS_TRACE("factory [%d] done\r\n", atoi(argv[1]));
	return 0;
}

OSAL_CMD(factory, 2, factory_test, "factory test");

int pop_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  
  if(!strcmp(argv[1], "src")){
		GPIO_PIN_SET(SRC4382_RST_PORT, SRC4382_RST_PIN, 0);
		SYS_TRACE("SRC4382_RST_PORT\r\n");
	}else if(!strcmp(argv[1], "pcm")){
		GPIO_PIN_SET(PCM_1795_RST_PORT, PCM_1795_RST_PIN, 0);
		SYS_TRACE("PCM_1795_RST_PORT\r\n");
	}
	return 0;
}

OSAL_CMD(pop, 2, pop_test, "factory test");




int upgrade_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	
	ConfigureUSBInterface();
	
	mdc_power_set(SLOT_1, OFF);
	
	bsp_delay_ms(100);
	
	mdc_power_set(SLOT_1, ON);
	
	osal_start_timerEx(gUpgradeTaskID, UPGRADE_POLL_SMG, 100);
	
 SYS_TRACE("\r\nupgrade_test start\r\n");
  
 return 0;
}

OSAL_CMD(upgrade, 1, upgrade_test, "upgrade test");
