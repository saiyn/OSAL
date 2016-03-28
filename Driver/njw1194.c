#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "njw1194.h"

/*Bit[3~0] chip address*/
#define CHIPADDR (0 << 0)

/*Bit[7~4] register address*/
#define VOLCTLA   (0<<4)
#define VOLCTLB   (1<<4)
#define INPUTSEL  (2<<4)
#define TREBLECLT (3<<4)
#define BASSCTL   (4<<4)

static void njw1195_delay(uint16 time)
{
	uint16 j;
	
	for(j = 0; j < time; j++);
}

static void njw1194_write(uint16 data)
{
	uint8 j;
	
	DISABLE_INTERRUPT();
	NJW1194_CS(0);
	NJW1194_CLK(1);
	njw1195_delay(20);
	
	for(j = 0; j < 16; j++){
		if(data & (0x8000 >> j)){
			NJW1194_DATA(1);
		}else{
			NJW1194_DATA(0);
		}
		NJW1194_CLK(1);
		njw1195_delay(40);
		NJW1194_CLK(0);
		njw1195_delay(20);
	}
	NJW1194_CLK(1);
	njw1195_delay(20);
	NJW1194_CS(1);
	njw1195_delay(50);
  ENABLE_INTERRUPT();
}

void njw1194_input_select(njw1194_channel_t channel)
{
	OSAL_ASSERT(channel <= CHANNEL_MAX);
	
	njw1194_write((channel << 10) | INPUTSEL | CHIPADDR);
}

void njw1194_treble_set(int vol)
{
	uint8 value;
	
	if(vol < 0){
		value = (uint8)(-vol);
		njw1194_write((value << 11) | (1 << 10) | TREBLECLT | CHIPADDR);
	}else{
		value = (uint8)vol;
		njw1194_write((1 << 15) | (value << 11) | (1 << 10) | TREBLECLT | CHIPADDR);
	}
	
}

void njw1194_bass_set(int vol)
{
	uint8 value;
	
	if(vol < 0){
		value = (uint8)(-vol);
		njw1194_write((value << 11) | BASSCTL | CHIPADDR);
	}else{
		value = (uint8)vol;
		njw1194_write((1 << 15) | (value << 11) | BASSCTL | CHIPADDR);
	}
	
}


void njw1194_volume_set(uint8 vol)
{
	njw1194_write((vol << 8) | VOLCTLA | CHIPADDR);
	njw1194_write((vol << 8) | VOLCTLB | CHIPADDR);
}

void njw1194_volume_l_set(uint8 vol)
{
	 njw1194_write((vol << 8) | VOLCTLB | CHIPADDR);
}

void njw1194_volume_r_set(uint8 vol)
{
	njw1194_write((vol << 8) | VOLCTLA | CHIPADDR);
}

void njw1194_mute(void)
{
	njw1194_volume_set(0xff);
}









