#include "sys_head.h"
#include "OSAL_Task.h"
#include "OSAL.h"
#include "OSAL_Timer.h"
#include "Display_Task.h"
#include "lcd_driver.h"
#include "OSAL_Console.h"
#include "Mdc.h"
#include "System_Task.h"
#include "Soft_Timer_Task.h"

#define LCD_LINE_LEN 16

uint8 gDisTaskID;

Dis_Msg_t *gDisplay;

menu_t *gMenuHead;

menu_t *cur_menu;

static const char *blu_status[MDC_BLU_STATUS_NUM]={
" ", "Connecting...", "Hotspot Mode"
};


void DisplayTaskInit(uint8 task_id)
{
	  gDisTaskID = task_id;
}

static bool dis_print(uint8 line, dis_align_t align, char *fmt, ...);
static void dis_print_arrow(uint8 line);

static void dis_source_menu_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	dis_print(LCD_LINE_1,ALIGN_LEFT, "%s", gCurSrc->src_name);
	if(sys->is_mute == true){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Volume:    Mute");
	}else{
		if(sys->master_vol >= 0){
			dis_print(LCD_LINE_2,ALIGN_LEFT, "Volume:  +%.1fdB", sys->master_vol);
		}else{
			dis_print(LCD_LINE_2,ALIGN_LEFT, "Volume:  %.1fdB", sys->master_vol);
		}
	}
	
	if(sys->src >= SRC_CONSTANT_NUM){
		if(gCurSrc->state == SRC_STATE_PLAY || gCurSrc->state == SRC_STATE_PAUSE){
			osal_start_timerEx(gDisTaskID, SRC_MENU_TIMEOUT_MSG,  2000);
		}else if(gCurSrc->state == SRC_STATE_FILE_LIST){
			osal_start_timerEx(gDisTaskID, SRC_MENU_TO_FILE_LIST_TIMEOUT_MSG,  2000);
		}
	}else if(sys->src == SRC_BT){
		/*added since v0.1.7 */
		osal_start_timerEx(gDisTaskID, SRC_MENU_TIMEOUT_MSG,  2000);
		
	}
}

static void dis_usb_file_list_menu_handler(sys_state_t *sys, uint8 para)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	/*check whether there is valid file to dispaly*/
	if(gcurrent_usb_file_cnt == 0){
		dis_print(LCD_LINE_1,ALIGN_CENTER, "%s", gCurSrc->src_name);
		dis_print(LCD_LINE_2,ALIGN_CENTER, "No Files Found");
	}else{
		
		if(gCurFile[p->mdc_slot_index] == NULL){
			dis_print(LCD_LINE_1,ALIGN_CENTER, "Reading Files...");
			dis_print(LCD_LINE_2,ALIGN_CENTER, "                ");
			return;
	}
		
	/*get here by calling sys_down_key_handler or back from play menu*/
	if(para == 0 || para == 2){
	
			/*when back from play menu, we need do this handler*/
			if(para == 2){
				dis_print(LCD_LINE_1,ALIGN_LEFT, "  %d.%s", gCurFile[p->mdc_slot_index]->prev->id, gCurFile[p->mdc_slot_index]->prev->file_name);
				dis_print(LCD_LINE_2,ALIGN_LEFT, "  %d.%s", gCurFile[p->mdc_slot_index]->id, gCurFile[p->mdc_slot_index]->file_name);
			}
			
			if(gCurFile[p->mdc_slot_index]->id % 2){
			/*just locate the arrow*/
			if(gCurFile[p->mdc_slot_index]->id % 2){
				dis_print_arrow(LCD_LINE_2);
			}else{
				dis_print_arrow(LCD_LINE_1);
			}
	
		}else{
		
			dis_print(LCD_LINE_1,ALIGN_LEFT, "  %d.%s", gCurFile[p->mdc_slot_index]->id, gCurFile[p->mdc_slot_index]->file_name);
			if(gCurFile[p->mdc_slot_index]->next){
				dis_print(LCD_LINE_2,ALIGN_LEFT, "  %d.%s", gCurFile[p->mdc_slot_index]->next->id, gCurFile[p->mdc_slot_index]->next->file_name);
			}else{
				dis_print(LCD_LINE_2,ALIGN_LEFT, "                ");
			}
		
			/*then locate the arrow*/
			if(gCurFile[p->mdc_slot_index]->id % 2){
				dis_print_arrow(LCD_LINE_2);
			}else{
				dis_print_arrow(LCD_LINE_1);
			}
		
	  }
	
  }else if(para == 1){
		/*get here by calling sys_up_key_handler*/
		if(gCurFile[p->mdc_slot_index]->id % 2){
			dis_print(LCD_LINE_1,ALIGN_LEFT, "  %d.%s", gCurFile[p->mdc_slot_index]->prev->id, gCurFile[p->mdc_slot_index]->prev->file_name);
			
			dis_print(LCD_LINE_2,ALIGN_LEFT, "  %d.%s", gCurFile[p->mdc_slot_index]->id, gCurFile[p->mdc_slot_index]->file_name);
			
			/*then locate the arrow*/
			if(gCurFile[p->mdc_slot_index]->id % 2){
				dis_print_arrow(LCD_LINE_2);
			}else{
				dis_print_arrow(LCD_LINE_1);
			}
			
		}else{
			
			/*just locate the arrow*/
			if(gCurFile[p->mdc_slot_index]->id % 2){
				dis_print_arrow(LCD_LINE_2);
			}else{
				dis_print_arrow(LCD_LINE_1);
			}
		}
	}

}
	
}

static void dis_start_scroll(uint8 line, char *buf)
{
	size_t len = strlen(buf);
	
	if(gSystem_t->sroll_buf){
		FREE(gSystem_t->sroll_buf);
		gSystem_t->sroll_buf = NULL;
	}
	gSrollBuf = MALLOC(len + 1);
	memset(gSrollBuf, 0, len + 1);
	strcpy(gSrollBuf, buf);
	gSystem_t->scroll_line = line;
	gSystem_t->sroll_buf = gSrollBuf;
	osal_start_timerEx(gSTTID, LCD_SCROLL_MSG,  1000);
	
}

void dis_scroll_update(char *buf)
{
	size_t len = strlen(buf);
	
	if(len <= 16) return;
	
	if(gSystem_t->sroll_buf){
		FREE(gSystem_t->sroll_buf);
		gSystem_t->sroll_buf = NULL;
	}
	gSrollBuf = MALLOC(len + 1);
	memset(gSrollBuf, 0, len + 1);
	strcpy(gSrollBuf, buf);
	gSystem_t->sroll_buf = gSrollBuf;
	
}

static void dis_usb_play_menu_handler(sys_state_t *sys, uint8 para)
{
	bool is_need_scroll;
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	is_need_scroll = dis_print(LCD_LINE_1,ALIGN_CENTER, "%s", gCurTitle[p->mdc_slot_index]);
	if(is_need_scroll){
		if(IS_Flag_Valid(DISPLAY_SCROLL_LOCK)){
			/*scroll not start, we need start now*/
			dis_start_scroll(LCD_LINE_1, gCurTitle[p->mdc_slot_index]);
			Deregister_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
			/*update current menu state*/
			display_menu_state_update(USB_PLAY_MENU, MENU_SCROLL);
		}
	}else{
		/*if the scroll had started, we need stop it now*/
		if(false == IS_Flag_Valid(DISPLAY_SCROLL_LOCK)){
			Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
		}
		
		/*update current menu state*/
	  display_menu_state_update(USB_PLAY_MENU, MENU_NORMAL);
	}
	
	if(para == 0){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Playing    %02d:%02d", gPlayTime / 60 , gPlayTime % 60);
	}else{
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Pause      %02d:%02d", gPlayTime / 60 , gPlayTime % 60);
	}
}

static void dis_usb_not_insert_menu_handler(sys_state_t *sys)
{
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	dis_print(LCD_LINE_1,ALIGN_CENTER, "%s", gCurSrc->src_name);
	
	dis_print(LCD_LINE_2,ALIGN_CENTER, "No Device Found");
}

static void dis_blu_status_menu_handler(sys_state_t *sys, uint8 para)
{
	dis_print(LCD_LINE_1,ALIGN_CENTER, "%s", gCurSrc->src_name);
	OSAL_ASSERT(para < MDC_BLU_STATUS_NUM);
	if(para == 0){
		dis_print(LCD_LINE_2,ALIGN_CENTER, "Volume  %.1fdB", sys->master_vol);
	}else{
		dis_print(LCD_LINE_2,ALIGN_CENTER, "%s", blu_status[para]);
	}
	
}

static void dis_output_select_menu(sys_state_t *sys, uint8 para)
{
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Speaker Output");
	if(sys->speak_output_mode == BYPASS){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Full Range");
	}else{
		dis_print(LCD_LINE_2,ALIGN_LEFT, "High Pass");
	}
	
	if(para == SPEAKER_OUT_NEED_UPDATE){
		sys_output_mode_select(sys);
	}
}

static void dis_line_out_select_menu(sys_state_t *sys, uint8 para)
{
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Pre Out/Subwoofe");
	if(sys->line_output_mode == BYPASS){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Pre Out");
	}else{
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Subwoofer");
	}
	
	if(para == LINE_OUT_NEED_UPDATE){
		sys_line_out_mode_select(sys);
	}
	
}

static void dis_ir_channel_select_menu_handler(sys_state_t *sys)
{
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	dis_print(LCD_LINE_1,ALIGN_LEFT, "IR Channel");
	
	dis_print(LCD_LINE_2,ALIGN_LEFT, "Channel %d", sys->ir_channel);
}

static void dis_auto_standby_menu_handler(sys_state_t *sys)
{
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Auto Standby");
	if(sys->is_auto_standby == false){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Off");
	}else{
		dis_print(LCD_LINE_2,ALIGN_LEFT, "On");
	}
}

static void dis_bt_status_menu_handler(sys_state_t *sys, uint8 para)
{
	dis_print(LCD_LINE_1,ALIGN_LEFT, "%s", gCurSrc->src_name);
	if(para == BT_LIMBO){
		//if(sys->is_mute == true){
		//	dis_print(LCD_LINE_2,ALIGN_CENTER, "Volume      Mute");
		//}else{
			/*changed since v0.1.7*/
			//dis_print(LCD_LINE_2,ALIGN_CENTER, "Volume  %.1fdB", sys->master_vol);
			dis_print(LCD_LINE_2,ALIGN_LEFT, "Undiscoverable");
		//}
	}else if(para == BT_CONNECTABLE){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Connectable");
	}else if(para == BT_CONNDISCOVERABLE){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Discoverable");
	}else if(para == BT_CONNECTED){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Connected");
	}else if(para == BT_A2DPSTREAMING){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Playing");
	}
	
}

static void dis_usb_reading_menu_handler(sys_state_t *sys)
{
	dis_print(LCD_LINE_1,ALIGN_CENTER, "Reading Files...");
	dis_print(LCD_LINE_2,ALIGN_CENTER, "                ");
}

static void dis_speaker_mode_select_menu_handler(sys_state_t *sys)
{
	/*first stop the srcoll*/
	Register_Bitmap_Flag(DISPLAY_SCROLL_LOCK);
	
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Speaker Channel");
	if(sys->speaker_mode == SPEAKER_A){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Speaker A");
	}else if(sys->speaker_mode == SPEAKER_B){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Speaker B");
	}else if(sys->speaker_mode == SPEAKER_A_B){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Speakers A + B");
	}else if(sys->speaker_mode == SPEAKER_OFF){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "Speakers Off");
	}
	
}



static void dis_dc_error_menu_handler(sys_state_t *sys)
{
	dis_print(LCD_LINE_1,ALIGN_CENTER, "PROTECTION!!!");
	dis_print(LCD_LINE_2,ALIGN_CENTER, "DC ERROR");
}

static void dis_over_temp_menu_handler(sys_state_t *sys)
{
	dis_print(LCD_LINE_1,ALIGN_CENTER, "PROTECTION!!!");
#ifdef NAD_C378
  if(IS_OVER_TEMP_L && IS_OVER_TEMP_R){
		dis_print(LCD_LINE_2,ALIGN_CENTER, "OVER TEMP L&R");
	}else if(IS_OVER_TEMP_L){
		dis_print(LCD_LINE_2,ALIGN_CENTER, "OVER TEMP LEFT");
	}else{
		dis_print(LCD_LINE_2,ALIGN_CENTER, "OVER TEMP RIGHT");
	}
	
#else
	
	dis_print(LCD_LINE_2,ALIGN_CENTER, "OVER TEMP");
#endif
}

static void dis_ac_error_menu_handler(sys_state_t *sys)
{
	dis_print(LCD_LINE_1,ALIGN_CENTER, "PROTECTION!!!");
	dis_print(LCD_LINE_2,ALIGN_CENTER, "AC ERROR");
}

static void dis_firmware_version_menu_handler(void)
{
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Firmware Version");
	dis_print(LCD_LINE_2,ALIGN_LEFT, "%s", version);
}

static void dis_treble_control_menu_handler(sys_state_t *sys)
{
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Treble");
	if(sys->treble_vol > 0){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "+%ddB", sys->treble_vol);
	}else if(sys->treble_vol < 0){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "%02ddB", sys->treble_vol);
	}else{
		dis_print(LCD_LINE_2,ALIGN_LEFT, "0dB");
	}
}

static void dis_bass_control_menu_handler(sys_state_t *sys)
{
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Bass");
	if(sys->bass_vol > 0){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "+%ddB", sys->bass_vol);
	}else if(sys->bass_vol < 0){
		dis_print(LCD_LINE_2,ALIGN_LEFT, "%02ddB", sys->bass_vol);
	}else{
		dis_print(LCD_LINE_2,ALIGN_LEFT, "0dB", sys->bass_vol);
	}
}


static void dis_balance_control_menu_handler(sys_state_t *sys)
{
	dis_print(LCD_LINE_1,ALIGN_LEFT, "Balance");
	dis_print(LCD_LINE_2,ALIGN_LEFT, "L: %ddB  R: %ddB", sys->balance_l, sys->balance_r);
}

extern mdc_slot_t gSlot;

static void dis_firmware_upgrade_menu_handler(sys_state_t *sys)
{
	mdc_t *p;
	
	p = (mdc_t *)sys->mdc;
	
	dis_print(LCD_LINE_1,ALIGN_LEFT, "MDC Card Upgrade");
	

	dis_print(LCD_LINE_2,ALIGN_LEFT, "Slot:%d  %s", gSlot + 1, CardTypeName[p->slot[gSlot].type]);

}

static void dispaly_update_handler(sys_state_t *sys, menu_id_t id, uint8 para)
{
	OSAL_ASSERT(sys != NULL);
	
	if(sys->status != STATUS_WORKING && sys->status != STAUS_INITING) return;
	
  gCurMenu = id;	
	
	switch(id){
		case SOURCE_MENU:
			dis_source_menu_handler(sys);
			break;
		
		case USB_FILE_LIST_MENU:
			dis_usb_file_list_menu_handler(sys, para);
			break;
		
		case USB_PLAY_MENU:
			dis_usb_play_menu_handler(sys, para);
			break;
		
		case USB_NOT_INSERT_MENU:
			dis_usb_not_insert_menu_handler(sys);
			break;
		
		case USB_READING_MENU:
			dis_usb_reading_menu_handler(sys);
			break;
		
		case BLU_STATUS_MENU:
			dis_blu_status_menu_handler(sys, para);
			break;
		
		case BT_STATUS_MENU:
			dis_bt_status_menu_handler(sys, para);
			break;
		
		case OUTPUT_SELECT_MENU:
			dis_output_select_menu(sys, para);
			break;
		
		case IR_CHANNEL_SELECT_MENU:
			dis_ir_channel_select_menu_handler(sys);
			break;
		
		case LINE_OUT_SELECT_MENU:
			dis_line_out_select_menu(sys, para);
			break;
		
		case AUTO_STANDBY_MENU:
			dis_auto_standby_menu_handler(sys);
			break;
		
		case DC_ERROR_MENU:
			dis_dc_error_menu_handler(sys);
			break;
		
		case OVER_TEMP_MENU:
			dis_over_temp_menu_handler(sys);
			break;
		
		case AC_ERROR_MENU:
			dis_ac_error_menu_handler(sys);
			break;
		
		case SPEAKE_MODE_SELECT_MENU:
			dis_speaker_mode_select_menu_handler(sys);
			break;
		
		case TREBEL_CONTROL_MENU:
			dis_treble_control_menu_handler(sys);
			break;
		
		case BASS_CONTROL_MENU:
			dis_bass_control_menu_handler(sys);
			break;

		case BALANCE_CONTROL_MENU:
			dis_balance_control_menu_handler(sys);
			break;
		
		case FIRMWARE_VERSION_MENU:
			dis_firmware_version_menu_handler();
			break;
		
		case FIRMWARE_UPGRADE_MENU:
			dis_firmware_upgrade_menu_handler(sys);
			break;
		
		
		default:
			break;
	}
	
}

static void vol_dis_timeout_handler(void)
{
	Deregister_Bitmap_Flag(DISPLAY_UPDATE_LOCK);
	if(gCurSrc->id == SRC_BT){
		Send_DisplayTask_Msg(DIS_UPDATE, gSystem_t->bt_s);
	}else{
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}
}

static void src_dis_timeout_handler(void)
{
	if(gCurSrc->state == SRC_STATE_PLAY && strstr(gCurSrc->src_name, "USB")){
		display_menu_jump(USB_PLAY_MENU);
	  Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}else if(gCurSrc->state == SRC_STATE_PAUSE && strstr(gCurSrc->src_name, "USB")){
		display_menu_jump(USB_PLAY_MENU);
	  Send_DisplayTask_Msg(DIS_UPDATE, 1);
	}else if(gCurSrc->id == SRC_BT){
		display_menu_jump(BT_STATUS_MENU);
		dis_bt_status_menu_handler(gSystem_t, gSystem_t->bt_s);
	}
	
}

static void src_menu_to_file_list_handler(void)
{
	if(strstr(gCurSrc->src_name, "USB") && gCurSrc->state == SRC_STATE_FILE_LIST){
		display_menu_jump(USB_FILE_LIST_MENU);
		Send_DisplayTask_Msg(DIS_UPDATE, 0);
	}
}


uint16 DisplayTaskEventLoop(uint8 task_id, uint16 events)
{
	   Dis_Msg_t *msg = NULL;
	
	   if(events & SYS_EVENT_MSG)
		 {
			  msg = (Dis_Msg_t *)osal_msg_receive(task_id);
			 
			  while(msg)
				{
					 switch(msg->hdr.event)
					 {
                case DIS_UPDATE:
									if(false == IS_Flag_Valid(DISPLAY_UPDATE_LOCK)){
										dispaly_update_handler(gSystem_t, cur_menu->id, msg->value);
									}
									break;
								
								case DIS_JUMP:
									dispaly_update_handler(gSystem_t, (menu_id_t)msg->value, 0);
									break;
								
								default:
									break;
					 }
					 
					 osal_msg_deallocate((uint8 *) msg);
					 
					 msg = (Dis_Msg_t *)osal_msg_receive(task_id);
				}
				
				return (events ^ SYS_EVENT_MSG);
		 }
		 
		 if(events & VOL_DIS_BACK_MSG){
			 vol_dis_timeout_handler();
			 return (events ^ VOL_DIS_BACK_MSG);
		 }
		 
		 if(events & SRC_MENU_TIMEOUT_MSG){
			 src_dis_timeout_handler();
			 return (events ^ SRC_MENU_TIMEOUT_MSG);
		 }
		 
		 if(events & SRC_MENU_TO_FILE_LIST_TIMEOUT_MSG){
			 
			 src_menu_to_file_list_handler();
			 
			 return(events ^ SRC_MENU_TO_FILE_LIST_TIMEOUT_MSG);
		 }
		 
		 return 0;
 }



void Send_DisplayTask_Msg(dis_event_t event, uint8 value)
{
	   uint8 result = 0;
		 
	   gDisplay = (Dis_Msg_t *)osal_msg_allocate(sizeof(Dis_Msg_t));
		 if(NULL == gDisplay)
     {
			 SYS_TRACE("no memory for dispaly msg\r\n");
			 return;
		 }
		 gDisplay->hdr.event = (uint8)event;	
		 gDisplay->value = value;
	 
		 result = osal_msg_send(gDisTaskID,(uint8 *)gDisplay);
		 if(result != SUCCESS)
		 {
			  SYS_TRACE("send display msg fail\r\n");
		 }
}
 
 

bool dis_print(uint8 line, dis_align_t align, char *fmt, ...)
{
	 uint8 j;
	 uint8 offset;
	 char buf[6*LCD_LINE_LEN] = {0};
   char temp[LCD_LINE_LEN + 1] = {0};
   va_list ap;
   size_t len;
   bool is_need_scroll = false;

   va_start(ap, fmt);
   vsprintf(buf, fmt, ap);
   va_end(ap);

   len = strlen(buf);

   if(len > LCD_LINE_LEN){
		 is_need_scroll = true;
		 if(false == IS_Flag_Valid(DISPLAY_SCROLL_LOCK)){
			 return is_need_scroll;
		 }
	 }

   if(align == ALIGN_CENTER){
		 offset =  LCD_LINE_LEN > len? (LCD_LINE_LEN - len):(0);
		 if(offset){
			 for(j = 0; j < offset/2; j++){
				 temp[j] = ' ';
			 }
			 
			 strcat(temp, buf);
		 }else{
			 strncpy(temp, buf, LCD_LINE_LEN);
		 }
		 
	 }else{
		 strncpy(temp, buf, LCD_LINE_LEN);
	 }

   lcd_write_cmd(line);
   for(j = 0; j < LCD_LINE_LEN; j++){
		 if(temp[j] == '<'){
			 lcd_write_data(0);
		 }else if(temp[j] == '>'){
			 lcd_write_data(5);
		 }else if(temp[j] == '~'){
			 lcd_write_data(6);
		 }else if(temp[j] == 0){
			 lcd_write_data(' ');
		 }else if(temp[j] == '^'){
			 lcd_write_data(4);
		 }else{
			 lcd_write_data(temp[j]);
		 }
	 }
   
	 return is_need_scroll;
}


void dis_simple_print(uint8 line, char *fmt, ...)
{
	 uint8 j;
	 uint8 offset;
	 char buf[6*LCD_LINE_LEN] = {0};
   char temp[LCD_LINE_LEN + 1] = {0};
   va_list ap;
   size_t len;

   va_start(ap, fmt);
   vsprintf(buf, fmt, ap);
   va_end(ap);

   len = strlen(buf);

   if(len > LCD_LINE_LEN){
		 
	 }

   if(0){
		 offset =  LCD_LINE_LEN > len? (LCD_LINE_LEN - len):(0);
		 if(offset){
			 for(j = 0; j < offset/2; j++){
				 temp[j] = ' ';
			 }
			 
			 strcat(temp, buf);
		 }else{
			 strncpy(temp, buf, LCD_LINE_LEN);
		 }
		 
	 }else{
		 strncpy(temp, buf, LCD_LINE_LEN);
	 }

   lcd_write_cmd(line);
   for(j = 0; j < LCD_LINE_LEN; j++){
		 if(temp[j] == '<'){
			 lcd_write_data(0);
		 }else if(temp[j] == '>'){
			 lcd_write_data(5);
		 }else if(temp[j] == '~'){
			 lcd_write_data(6);
		 }else if(temp[j] == 0){
			 lcd_write_data(' ');
		 }else if(temp[j] == '^'){
			 lcd_write_data(4);
		 }else{
			 lcd_write_data(temp[j]);
		 }
	 }
   

}

void dis_print_scroll(uint8 line)
{
	char buf[LCD_LINE_LEN + 1] = {0};
  size_t size = 0;	
  uint8 len = 0;
  uint8 j;

	OSAL_ASSERT(gSrollBuf != NULL);

  size = strlen(gSrollBuf);

  //SYS_TRACE("dis_print_scroll:[%d]->%s\r\n", size, gSrollBuf);

  len = size > LCD_LINE_LEN ? LCD_LINE_LEN: size;
	
	strncpy(buf, gSrollBuf, len);
	
	if(len > 1){
		gSrollBuf++;
	}else{
		gSrollBuf = gSystem_t->sroll_buf;
	}
	
	lcd_write_cmd(line);
	for(j = 0; j < LCD_LINE_LEN; j++){
		 if(buf[j] == 0){
			 lcd_write_data(' ');
		 }else{
			 lcd_write_data(buf[j]);
		 }
	 }
	
}
 
static void dis_print_arrow(uint8 line)
{
	lcd_write_cmd(line);
	lcd_write_data(6);
	lcd_write_data(7);
	if(line == LCD_LINE_1){
		lcd_write_cmd(LCD_LINE_2);
		lcd_write_data(' ');
	  lcd_write_data(' ');
	}else{
		lcd_write_cmd(LCD_LINE_1);
		lcd_write_data(' ');
	  lcd_write_data(' ');
	}
}
 
void display_init_menu(void)
{ 
#ifdef NAD_C378
	dis_print(LCD_LINE_1,ALIGN_CENTER, "NAD C378");
#else
	dis_print(LCD_LINE_1,ALIGN_CENTER, "NAD C358");
#endif
	
	dis_print(LCD_LINE_2,ALIGN_CENTER, "                ");
}

void display_factory_menu(void)
{
	dis_print(LCD_LINE_1,ALIGN_CENTER, "Factory Reset");
	dis_print(LCD_LINE_2,ALIGN_CENTER, "                ");
}

static menu_t *menu_list_add_next(menu_t **head, menu_id_t id)
{
	menu_t *menu;
	menu_t *p = *head;
	
	menu = (menu_t *)MALLOC(sizeof(menu_t));
	
	OSAL_ASSERT(menu != NULL);
	
	menu->id = id;
	menu->state = MENU_NORMAL;
	menu->child = NULL;
	menu->parent = NULL;
	menu->next = NULL;
	menu->prev = NULL;
	
	if(*head){
		while(p->next){
			p = p->next;
		}
		
		p->next = menu;
		menu->prev = p;
		
	}else{
		*head = menu;
	}
	
	return menu;
}

void display_menu_list_create(menu_t **head)
{
	uint8 j;
	menu_t *tail;
	menu_t *found;
	
	for(j = 0; j < MENU_NUM; j++){
		tail = menu_list_add_next(head, (menu_id_t)j);
	}
	
	/*make the OUTPUT_SELECT_MENU prev pointer point to the last menu*/
	found = *head;
	while(found){
		if(found->id == OUTPUT_SELECT_MENU){
			break;
		}else{
			found = found->next;
		}
	}
	
	found->prev = tail;
}

void display_menu_jump(menu_id_t to)
{
	menu_t *found = gMenuHead;
	
	if(cur_menu->id == to) return;
	
	while(found){
		if(found->id == to){
			cur_menu = found;
			return;
		}else{
			found = found->next;
		}
	}
	
}

void display_menu_state_update(menu_id_t id, menu_state_t state)
{
	menu_t *found = gMenuHead;
	
	while(found){
		if(found->id == id){
			found->state = state;
			return;
		}else{
			found = found->next;
		}
	}
	
}


 
 
int dis(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	display_init_menu();
	return 0;
}

OSAL_CMD(dis, 1, dis, "lcd init test");
 
 
 
 
 
 
 
 


