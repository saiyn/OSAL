#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Key.h"
#include "UI.h"
#include "Mdc.h"
#include "System_Task.h"

#define NUM_OF_KEY 8

typedef struct
{
	uint8 (*pkeyread)(key_value_t key);
	key_value_t shortpress;
	key_value_t longpress;
  uint32 longpresstime;
	bool repeat_flag;
	bool key_locked_flag;
	uint32 key_timer;
}key_control_t;


bool osal_key_is_down(key_value_t key)
{
	uint8 val = ui_key_read();
	
	if(val & (1 << key)){
		return true;
	}else{
		return false;
	}
	
}

static uint8 key_read(key_value_t key)
{
	uint8 val = ui_key_read();
	
	if(val & (1 << key)){
		return 1;
	}else{
		return 0;
	}
	
}

key_control_t keylist[NUM_OF_KEY] =
{
	
{key_read,KEY_UP, KEY_UP_LONG,5000,false,false,0},
{key_read,KEY_DOWN, KEY_DOWN_LONG,5000,false,false,0},
{key_read,KEY_LEFT, KEY_LEFT_LONG,5000,false,false,0},
{key_read,KEY_RIGHT, KEY_RIGHT_LONG,1000,false,false,0},
{key_read,KEY_STANDBY, KEY_STANDBY_LONG,1000,false,false,0},
{key_read,KEY_SRC_NEXT, KEY_SRC_NEXT_LONG,3000,false,false,0},
{key_read,KEY_SRC_PREV, KEY_SRC_PREV_LONG,3000,false,false,0},
{key_read,KEY_ENTER, KEY_ENTER_LONG,3000,false,false,0},
           
};


void KeyScanLoop(void)
{
	unsigned char index = 0;
	unsigned int temp;
	
  for(index = 0; index < NUM_OF_KEY; index++)
	{
		 if(keylist[index].pkeyread(keylist[index].shortpress) == 1)
		 {
			  if((keylist[index].key_locked_flag == false) || (keylist[index].repeat_flag == true))
				{
					    if(keylist[index].key_locked_flag)
							{
								  temp = 200;
							}
							else
							{
								  temp = keylist[index].longpresstime;
							}
							
							if(gTickCount - keylist[index].key_timer > temp)
							{
								if(index == KEY_STANDBY)
                   Send_SysTask_Msg(SYS_POWER_MSG, (uint8)keylist[index].longpress);  	
                else							
						       Send_SysTask_Msg(SYS_KEYBOARD_MEG, (uint8)keylist[index].longpress);
								
								  keylist[index].key_locked_flag = true;
								  keylist[index].key_timer = gTickCount;
							}
				}
				else if(keylist[index].key_locked_flag == true)
				{
					  keylist[index].key_timer = gTickCount;
				}
		 }
		 else 
		 {
			    if(gTickCount - keylist[index].key_timer > 50)
					{
						  if(index == KEY_STANDBY)
                   Send_SysTask_Msg(SYS_POWER_MSG, (uint8)keylist[index].shortpress);  	
              else							
						       Send_SysTask_Msg(SYS_KEYBOARD_MEG, (uint8)keylist[index].shortpress);
					}
					
					keylist[index].key_locked_flag = false;
					keylist[index].key_timer = gTickCount;
		 }
		 
	 }
	
	
}










