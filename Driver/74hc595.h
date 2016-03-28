#ifndef _74HC595_H_
#define _74HC595_H_



void hc595_write(hc595_io_t io, uint8 val);



int hc595_init(void);


#define SPEAKER1_MUTE_ON()  hc595_write(HC595_PS5, 0) 
#define SPEAKER1_MUTE_OFF()  hc595_write(HC595_PS5, 1) 
#define SPEAKER2_MUTE_ON()  hc595_write(HC595_PS4, 0) 
#define SPEAKER2_MUTE_OFF()  hc595_write(HC595_PS4, 1)
#define HP_MUTE_ON() hc595_write(HC595_HP_MUTE, 0) 
#define HP_MUTE_OFF() hc595_write(HC595_HP_MUTE, 1) 
#define LINE_MUTE_ON()  hc595_write(HC595_LINE_MUTE, 0) 
#define LINE_MUTE_OFF()  hc595_write(HC595_LINE_MUTE, 1) 
#define SPEAKER_SUB_OUTPUT() hc595_write(HC595_PS1, 1) 
#define SPEAKER_LFE_OUTPUT() hc595_write(HC595_PS1, 0)
#define LINE_SUB_OUT()   hc595_write(HC595_PS2, 1)
#define LINE_BYPASS()    hc595_write(HC595_PS2, 0)
#define HC595_PS_EN(x)   hc595_write(HC595_PS_EN, x)    

#endif



