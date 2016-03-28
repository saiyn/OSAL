#include "sys_head.h"
#include "OSAL_Task.h"
#include "OSAL.h"
#include "OSAL_Detect.h"
#include "Mdc.h"
#include "System_Task.h"
#include "Src4382.h"


detect_control_t detectlist[NUM_OF_DETECT] =
{
{GPIO_ReadSinglePin, TRIGGER_IN_PORT, TRIGGER_IN_PIN, 0, DETECT_BEGIN, sys_trigger_plug_in_handler, sys_trigger_plug_out_handler},
{GPIO_ReadSinglePin, TRIGGER_LEVEL_PORT, TRIGGER_LEVEL_PIN, 0, DETECT_BEGIN, sys_trigger_valid_handler, sys_trigger_invalid_handler},
{GPIO_ReadSinglePin, HP_INSERT_DETECT_PORT, HP_INSERT_DETECT_PIN, 1, DETECT_BEGIN, sys_hp_plug_in_handler, sys_hp_plug_out_handler},
{GPIO_ReadSinglePin, AC_DETECT_PORT, AC_DETECT_PIN, 0, DETECT_BEGIN, sys_protection_valid_handler, sys_protection_invalid_handler},
{GPIO_ReadSinglePin, DC_ERROR_PORT, DC_ERROR_PIN, 1, DETECT_BEGIN, sys_protection_valid_handler, sys_protection_invalid_handler},
#ifdef NAD_C378
{GPIO_ReadSinglePin, DC_ERROR2_PORT, DC_ERROR2_PIN, 1, DETECT_BEGIN, sys_protection_valid_handler, sys_protection_invalid_handler},
#endif

{GPIO_ReadSinglePin, OVER_TEMP_R_PORT, OVER_TEMP_R_PIN, 1, DETECT_BEGIN, sys_protection_valid_handler, sys_protection_invalid_handler},
#ifdef NAD_C378
{GPIO_ReadSinglePin, OVER_TEMP_L_PORT, OVER_TEMP_L_PIN, 1, DETECT_BEGIN, sys_protection_valid_handler, sys_protection_invalid_handler},
#endif

{GPIO_ReadSinglePin, AUDIO_SENSE_PORT, AUDIO_SENSE_PIN, 1, DETECT_BEGIN, sys_audio_valid_handler, sys_audio_invalid_handler},
{src4382_audio_detect, 0,0, 1, DETECT_BEGIN, sys_digital_audio_valid_handler, sys_digital_audio_invalid_handler}
};

void update_detect_state(detect_event_t event)
{
	OSAL_ASSERT(event < NUM_OF_DETECT);
	
	detectlist[event].state = DETECT_BEGIN;
}

void update_all_protect_state(void)
{
	detect_event_t event;
	
	for(event = DETECT_AC_DET; event < DETECT_AUDIO; event++){
		detectlist[event].state = DETECT_BEGIN;
	}
	 
}

void set_detect_state(detect_event_t event, detect_state_t state)
{
	OSAL_ASSERT(event < NUM_OF_DETECT);
	
	detectlist[event].state = state;
}


void DetectGpioPoll(void)
{
	uint8 index;
	
	for(index = 0; index < NUM_OF_DETECT; index++){ 
		
		if(index >= DETECT_AC_DET){
			if(gSystem_t->status != STATUS_WORKING){
				return;
			}
		}

		switch(detectlist[index].state){
			case DETECT_BEGIN:
				if(detectlist[index].pstateread(detectlist[index].port, detectlist[index].pin) == detectlist[index].mode){
					// vTaskDelay(100/portTICK_RATE_MS);
				   if(detectlist[index].pstateread(detectlist[index].port, detectlist[index].pin) == detectlist[index].mode){
							if(detectlist[index].validstatecallback != NULL){
								detectlist[index].validstatecallback(index);
								/*when protection valid we will delay sometime to check invalid*/
								if(index >= DETECT_AC_DET && index < DETECT_AUDIO){
										detectlist[index].state = DETECT_STOP;
								}else{
									  detectlist[index].state = DETECT_VALID;
								}
							}
					 }else{
						  if(detectlist[index].invalidstatecallback != NULL){
				        detectlist[index].invalidstatecallback(index);
			        }
								detectlist[index].state = DETECT_INVALID;
					 }
				 }else{
					  if(detectlist[index].invalidstatecallback != NULL){
				        detectlist[index].invalidstatecallback(index);
			        }
								detectlist[index].state = DETECT_INVALID;
				 }
				break;
				
			case DETECT_VALID:
				if(detectlist[index].pstateread(detectlist[index].port, detectlist[index].pin) != detectlist[index].mode){
					if(detectlist[index].invalidstatecallback != NULL){
				     detectlist[index].invalidstatecallback(index);
			     }
					 detectlist[index].state = DETECT_INVALID;
				}
				break;
				
			case DETECT_INVALID:
				if(detectlist[index].pstateread(detectlist[index].port, detectlist[index].pin) == detectlist[index].mode){
					//vTaskDelay(100/portTICK_RATE_MS);
					if(detectlist[index].pstateread(detectlist[index].port, detectlist[index].pin) == detectlist[index].mode){
							if(detectlist[index].validstatecallback != NULL){
								detectlist[index].validstatecallback(index);
								/*when protection valid we will delay sometime to check invalid*/
								if(index >= DETECT_AC_DET && index < DETECT_AUDIO){
										detectlist[index].state = DETECT_STOP;
								}else{
									  detectlist[index].state = DETECT_VALID;
								}
				   }
				 }
			 }
				break;
			
			default:
				break;
		}
}

}











