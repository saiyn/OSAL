#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "UI.h"
#include "Mdc.h"
#include "System_Task.h"

#define KEY_STB(x) GPIO_PIN_SET(KEY_STB_PORT, KEY_STB_PIN, x)
#define KEY_SCK(x) GPIO_PIN_SET(KEY_SCK_PORT, KEY_SCK_PIN, x)
#define KEY_MISO_IN() GPIO_PIN_GET(KEY_MISO_PORT, KEY_MISO_PIN)

#define ENCODE_A_IN() GPIO_PIN_GET(ENCODE_A_PORT, ENCODE_A_PIN)

#define ENCODE_THRESHOLD   1

int ui_init(void)
{
	/*init hc165*/
	KEY_STB(0);
	KEY_SCK(1);
	
	/*enable encode interrupt*/
	GPIOIntEnable(ENCODE_B_PORT, ENCODE_B_PIN);
	
	return 0;
}

uint8 ui_key_read(void)
{
	uint8 retval = 0;
	uint8 i;
	
	KEY_STB(0);
	NOP();
	KEY_STB(1);
	NOP();
	
	for(i = 0; i < 8; i++){
		if(!KEY_MISO_IN()){
			retval |= (1 << i);
		}
		KEY_SCK(0);
		NOP();
		KEY_SCK(1);
		NOP();
	}
	
	return retval;
}

void encode_interrupt_handler(void)
{
	 uint32 status;
	 static uint32 cnt=0;
	 uint32 time_cnt;
	 uint32 time_cur;
	 static uint32 time_last = 0;
	 uint8 step = 0;
	
	 status = GPIOIntStatus(ENCODE_B_PORT, 1);
	 GPIOIntClear(ENCODE_B_PORT, status);
	
	//SYS_TRACE("in encode_interrupt_handler\r\n");
	time_cur = bsp_timer0_get_time();
	time_cnt = time_cur - time_last;
	time_last = time_cur;
	if(time_cnt < 300){
		step = 2;
	}else if(time_cnt < 600){
		step = 1;
	}
	
	 //if(cnt++ >= ENCODE_THRESHOLD){
		 if(!ENCODE_A_IN()){
			 //SYS_TRACE("vol up[%d]\r\n", time_cnt);
			 Send_SysTask_Msg(SYS_VOL_UP, step);
		 }else{
			 Send_SysTask_Msg(SYS_VOL_DOWN, step);
			 //SYS_TRACE("vol down[%d]\r\n", time_cnt);
		 }
		 
//		 cnt = 0;
//	 }
	 
}


void ui_led_control(led_state_t state)
{
	 switch(state){
		 case LED_OFF:
			 GPIO_PIN_SET(LEDA_PORT, LEDA_PIN, 1);
		   GPIO_PIN_SET(LEDB_PORT, LEDB_PIN, 1);
			 break;
		 
		 case LED_ORANGE:
			 GPIO_PIN_SET(LEDA_PORT, LEDA_PIN, 0);
		   GPIO_PIN_SET(LEDB_PORT, LEDB_PIN, 0);
			 break;
		 
		 case LED_RED:
			 GPIO_PIN_SET(LEDA_PORT, LEDA_PIN, 0);
		   GPIO_PIN_SET(LEDB_PORT, LEDB_PIN, 1);
			 break;
		 
		 case LED_BIUE:
			 GPIO_PIN_SET(LEDA_PORT, LEDA_PIN, 1);
		   GPIO_PIN_SET(LEDB_PORT, LEDB_PIN, 0);
			 break;
		 
		 default:
			 break;
	 }
}






















