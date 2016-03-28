#ifndef _OSAL_DETECT_H_
#define _OSAL_DETECT_H_


typedef enum{
	DETECT_BEGIN=0,
	DETECT_VALID,
	DETECT_INVALID,
	DETECT_STOP,
}detect_state_t;

typedef struct
{
	uint8 (*pstateread)(uint32 GPIOx, uint8 GPIO_Pin);
  uint32 port;
  uint8 pin;
  uint8 mode;
  detect_state_t state;
  void (*validstatecallback)(uint8 );
  void (*invalidstatecallback)(uint8);  
}detect_control_t;

void update_all_protect_state(void);

void update_detect_state(detect_event_t event);

void set_detect_state(detect_event_t event, detect_state_t state);

void DetectGpioPoll(void);


void update_all_protect_state(void);


#endif



