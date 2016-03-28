#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Timer.h"
#include "OSAL_RingBuf.h"
#include "IR_driver.h"
#include "Mdc.h"
#include "System_Task.h"
#include "Soft_Timer_Task.h"

//100us
#define LEADER_TIME_MAX  (149)
#define LEADER_TIME_MIN  (124)

#define REPEAT_TIME_MAX  (130)
#define REPEAT_TIME_MIN  (100)

#define ZERO_TIME_MIN	(8)
#define ONE_TIME_MAX	(30)
#define MEDIAN_BIT_TIME	(17)

#define AMP_CUSTMER_CODE_LB 0x87
#define AMP_CUSTMET_CODE_HB 0x7c

#define MP_CUSTMER_CODE_LB 0x86
#define MP_CUSTMET_CODE_HB 0x0F

#define SET_IR_OUT(x) GPIO_PIN_SET(IR_OUT_PORT, IR_OUT_PIN, x)

ring_buffer_t ir_front;
ring_buffer_t ir_in;
volatile uint8 repeat_commond;
volatile uint8 repeat_thres;
uint16 gIr_timeout;

static ir_control_t ir;
static ir_control_t ir_jack;

uint8 ir_commond[IR_COMMOND_NUM]={
0x88,//IR_VOL_UP =0,
0x8c,//IR_VOL_DOWN,
0x25,//IR_ON,
0xc8,//IR_OFF,
0x1a,//IR_SOURCE_UP,
0x1d,//IR_SOURCE_DOWN,
0x94,//IR_MUTE,
0x6c,//IR_PAUSE,
0x71,//IR_PREV,
0x6b,//IR_PLAY,
0x20,//IR_RTN,
0x27,//IR_UP,
0x29,//IR_LFET,
0x32,//IR_CENTER,
0x2a,//IR_RIGHT,
0x28,//IR_DOWN,
0x69,//IR_REPEAT,
0x68,//IR_SHUFFLE,
0x70,//IR_NEXT,
0xc2,//IR_NUM1,
0xc0,//IR_NUM2,
0xc1,//IR_NUM3,
0x30,//IR_NUM4,
0x31,//IR_NUM5,
0x1e,//IR_NUM6,
0x60,//IR_NUM7,
0x85,//IR_NUM8,
0x89,//IR_NUM9,
0x9b,//IR_NUM10,
0x38,//IR_DIMMER,
0xc9,//IR_EQ,
0x39,//IR_TONE,
0x7f//IR_MENU,
};

void ir_in_recive_handler(void)
{
	uint32 status;
	uint32 time_cnt;
	uint32 time_cur;
	
	status = GPIOIntStatus(IR_IN_PORT, 1);
	GPIOIntClear(IR_IN_PORT, status);
	
	/*prevent the polarity check work*/
	osal_start_timerEx(gSTTID, IR_IN_POLARITY_CHECK_MSG, 150);
	
	time_cur = bsp_timer0_get_time();
	time_cnt = time_cur - ir_jack.last_time;
	ir_jack.last_time = time_cur;
	
	if(time_cnt <= 1) return;
	
	gIr_timeout = 0;
	switch(ir_jack.state){
		case NEC_IDLE:
		   ir_jack.state = NEC_LEADER;
			break;
		
		case NEC_LEADER:
			if(time_cnt < LEADER_TIME_MAX && time_cnt > LEADER_TIME_MIN){
				ir_jack.code.val = 0;
				ir_jack.bitcnt = 0;
				ir_jack.state = NEC_CODE;
			}else{
				ir_jack.state = NEC_IDLE;
			}
			break;
			
		case NEC_CODE:
			if(time_cnt < ONE_TIME_MAX && time_cnt > ZERO_TIME_MIN){
				ir_jack.bitcnt++;
				ir_jack.code.val >>= 1;
				if(time_cnt >= MEDIAN_BIT_TIME){
					ir_jack.code.val |= 0x80000000;
				}
			}
			
			if(ir_jack.bitcnt == 32){
				//SYS_TRACE("jack ir code =[%x],LB=%2x, HB=%2x, UB=%2x, MB=%2x \r\n", ir_jack.code.val, ir_jack.code.byte.LB, ir_jack.code.byte.HB, ir_jack.code.byte.UB, ir_jack.code.byte.MB);
				if(((ir_jack.code.byte.LB == AMP_CUSTMER_CODE_LB) && (ir_jack.code.byte.HB == AMP_CUSTMET_CODE_HB)) 
					|| ((ir_jack.code.byte.LB == MP_CUSTMER_CODE_LB) && (ir_jack.code.byte.HB == MP_CUSTMET_CODE_HB))){
					if(((ir_jack.code.byte.MB ^ ir_channel[gSystem_t->ir_channel]) == ir_jack.code.byte.UB)){
						ring_buffer_write(IR_BACK_IN, ir_jack.code.byte.UB);
						ir_jack.bitcnt = 0;
						ir_jack.state = NEC_REPEATE1;
						if((ir_commond[IR_VOL_UP] == ir_jack.code.byte.UB) || (ir_commond[IR_LFET] == ir_jack.code.byte.UB) || (ir_commond[IR_RIGHT]== ir_jack.code.byte.UB) || (ir_commond[IR_VOL_DOWN] == ir_jack.code.byte.UB)){
							
							repeat_thres = 5;
							repeat_commond = ir_jack.code.byte.UB;
						}else{
							repeat_thres = 0;
							repeat_commond = 0;
						}
						
					  ir_jack.code.val = 0;
					}else{
						ir_jack.state = NEC_IDLE;
					}
				}else{
					ir_jack.state = NEC_IDLE;
				}
			}
			break;
			
		case NEC_REPEATE1:
			ir_jack.state = NEC_REPEATE2;
			break;
		
		case NEC_REPEATE2:
			if(time_cnt <= REPEAT_TIME_MAX && time_cnt >= REPEAT_TIME_MIN){
				if(repeat_commond){
					ir_jack.state = NEC_REPEATE1;
					if(repeat_thres){
						repeat_thres--;
					}else{
						ring_buffer_write(IR_BACK_IN, repeat_commond);
					}
				}else{
					ir_jack.state = NEC_IDLE;
				}
			}
			break;
		
		default:
			break;
	}
	
}


void ir_front_recive_handler(void)
{
	uint32 status;
	uint32 time_cnt;
	uint32 time_cur;
	
	status = GPIOIntStatus(FRONT_IR_PORT, 1);
	GPIOIntClear(FRONT_IR_PORT, status);
	
	//if(IS_IR_JACK_IN()) return;
	
	time_cur = bsp_timer0_get_time();
	time_cnt = time_cur - ir.last_time;
	ir.last_time = time_cur;
	
	gIr_timeout = 0;
	switch(ir.state){
		case NEC_IDLE:
			   
		   ir.state = NEC_LEADER;
			break;
		
		case NEC_LEADER:
			if(time_cnt < LEADER_TIME_MAX && time_cnt > LEADER_TIME_MIN){
				ir.code.val = 0;
				ir.bitcnt = 0;
				ir.state = NEC_CODE;
			}else{
				ir.state = NEC_IDLE;
			}
			break;
			
		case NEC_CODE:
			if(time_cnt < ONE_TIME_MAX && time_cnt > ZERO_TIME_MIN){
				ir.bitcnt++;
				ir.code.val >>= 1;
				if(time_cnt >= MEDIAN_BIT_TIME){
					ir.code.val |= 0x80000000;
				}
			}
			
			if(ir.bitcnt == 32){
				//SYS_TRACE("ir code =[%x],LB=%2x, HB=%2x, UB=%2x, MB=%2x \r\n", ir.code.val, ir.code.byte.LB, ir.code.byte.HB, ir.code.byte.UB, ir.code.byte.MB);
				if(((ir.code.byte.LB == AMP_CUSTMER_CODE_LB) && (ir.code.byte.HB == AMP_CUSTMET_CODE_HB)) 
					|| ((ir.code.byte.LB == MP_CUSTMER_CODE_LB) && (ir.code.byte.HB == MP_CUSTMET_CODE_HB))){
					if(((ir.code.byte.MB ^ ir_channel[gSystem_t->ir_channel]) == ir.code.byte.UB)){
						ring_buffer_write(IR_FRONT, ir.code.byte.UB);
						ir.bitcnt = 0;
						ir.state = NEC_REPEATE1;
						if((ir_commond[IR_VOL_UP] == ir.code.byte.UB) || (ir_commond[IR_LFET] == ir.code.byte.UB) || (ir_commond[IR_RIGHT]== ir.code.byte.UB) || (ir_commond[IR_VOL_DOWN] == ir.code.byte.UB)){
							
							repeat_thres = 5;
							repeat_commond = ir.code.byte.UB;
						}else{
							repeat_thres = 0;
							repeat_commond = 0;
						}
						
					  ir.code.val = 0;
					}else{
						ir.state = NEC_IDLE;
					}
				}else{
					ir.state = NEC_IDLE;
				}
			}
			break;
			
		case NEC_REPEATE1:
			ir.state = NEC_REPEATE2;
			break;
		
		case NEC_REPEATE2:
			if(time_cnt <= REPEAT_TIME_MAX && time_cnt >= REPEAT_TIME_MIN){
				if(repeat_commond){
					ir.state = NEC_REPEATE1;
					if(repeat_thres){
						repeat_thres--;
					}else{
						ring_buffer_write(IR_FRONT, repeat_commond);
					}
				}else{
					ir.state = NEC_IDLE;
				}
			}
			break;
		
		default:
			break;
	}
	
}


bool ir_get_commond(ringbuf_device_t device, ir_commond_t *commond)
{
	uint8 j;
	uint8 code;
	
	OSAL_ASSERT(device < DEVICE_NUM);
	
	if(ring_buffer_len(device) == 0) return false;
	
	code = ring_buffer_read(device);
	
	//SYS_TRACE("check ir code = %2x\r\n", code);
	
	for(j = 0; j < IR_COMMOND_NUM; j++){
		if(code == ir_commond[j]){
			*commond = (ir_commond_t)j;
			return true;
		}
	}
	
	return false;
}

void ir_timeout_check(void)
{
	if(++gIr_timeout >= 130){
		gIr_timeout = 0;
		ir.bitcnt = 0;
		ir.code.val = 0;
		ir.state = NEC_IDLE;
		
		ir_jack.bitcnt = 0;
		ir_jack.code.val = 0;
		ir_jack.state = NEC_IDLE;
		
		repeat_commond = 0;
		repeat_thres = 0;
	}
}


#if 0
void IrOutHandler(uint8 command)
{
	uint8 addr_low = CUSTMER_CODE_LB;
	uint8 addr_high = CUSTMET_CODE_HB;
	uint8 index;
	
	SET_IR_OUT(0);
	bsp_delay_us(9000);
	SET_IR_OUT(1);
	bsp_delay_us(4500);
	
	for(index = 0; index < 8; index++)
	{		
	   SET_IR_OUT(0); 
		 bsp_delay_us(560);
		 SET_IR_OUT(1);
		 if(addr_low & (1 << index))
		 {			  
			  bsp_delay_us(1690);
		 }
		 else
		 {
			  bsp_delay_us(560);
		 }
		 
	}
	
		for(index = 0; index < 8; index++)
	{		
	   SET_IR_OUT(0); 
		 bsp_delay_us(560);
		 SET_IR_OUT(1);
		 if(addr_high & (1 << index))
		 {			  
			  bsp_delay_us(1690);
		 }
		 else
		 {
			  bsp_delay_us(560);
		 }
		 
	}
	
		for(index = 0; index < 8; index++)
	{		
	   SET_IR_OUT(0); 
		 bsp_delay_us(560);
		 SET_IR_OUT(1);
		 if(command & (1 << index))
		 {			  
			  bsp_delay_us(1690);
		 }
		 else
		 {
			  bsp_delay_us(560);
		 }
		 
	}
	
		for(index = 0; index < 8; index++)
	{		
	   SET_IR_OUT(0); 
		 bsp_delay_us(560);
		 SET_IR_OUT(1);
		 if((~command) & (1 << index))
		 {			  
			  bsp_delay_us(1690);
		 }
		 else
		 {
			  bsp_delay_us(560);
		 }
		 
	}
	
  SET_IR_OUT(0); 
	bsp_delay_us(560);
	SET_IR_OUT(1);
	
}

#endif








