#ifndef _IR_DRIVER_H_
#define _IR_DRIVER_H_



typedef enum{
	NEC_IDLE = 0,
	NEC_LEADER,
	NEC_CODE,
	NEC_REPEATE1,
	NEC_REPEATE2
}ir_state_t;

typedef union{
	uint32 val;
	struct{
		uint8 LB;
		uint8 HB;
		uint8 UB;
		uint8 MB;
	}byte;
}ir_code_t;

typedef struct{
	ir_state_t state;
	uint8 bitcnt;
	ir_code_t code;
	uint32 last_time;
}ir_control_t;



bool ir_get_commond(ringbuf_device_t device, ir_commond_t *commond);


extern uint16 gIr_timeout;

extern uint8 ir_commond[IR_COMMOND_NUM];

void ir_timeout_check(void);

void IrOutHandler(uint8 command);


#endif




