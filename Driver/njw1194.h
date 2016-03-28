#ifndef _NJW1194_H_
#define _NJW1194_H_



#define NJW1194_DATA(x)   GPIO_PIN_SET(NJW1194_MOSI_PORT, NJW1194_MOSI_PIN, x)   
#define NJW1194_CLK(x)    GPIO_PIN_SET(NJW1194_CLK_PORT, NJW1194_CLK_PIN, x)
#define NJW1194_CS(x)     GPIO_PIN_SET(NJW1194_CS_PORT, NJW1194_CS_PIN, x)


typedef enum{
	MUTE =0,
	FROM_MM,
	FROM_LINE,
	FROM_DAC,
	CHANNEL_MAX
}njw1194_channel_t;


void njw1194_input_select(njw1194_channel_t channel);


void njw1194_volume_set(uint8 vol);

void njw1194_treble_set(int vol);

void njw1194_bass_set(int vol);

void njw1194_mute(void);

void njw1194_volume_l_set(uint8 vol);

void njw1194_volume_r_set(uint8 vol);

#endif


