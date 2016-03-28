#include "sys_head.h"
#include "74hc595.h"


#define HC595_DATA(x) GPIO_PIN_SET(HC595_DATA_PORT, HC595_DATA_PIN, x)

#define HC595_CLK(x)  GPIO_PIN_SET(HC595_CLK_PORT, HC595_CLK_PIN, x)

#define HC595_CS(x)   GPIO_PIN_SET(HC595_CS_PORT, HC595_CS_PIN, x)

static uint8 hc595_value = 0;


int hc595_init(void)
{
	uint8 j;
	
	//hc595_value = (1 << HC595_PS_EN );
	
	hc595_value = 0;
	
	for(j = 0; j < 8; j++){
		if(hc595_value & (0x80 >> j)){
			HC595_DATA(1);
		}else{
			HC595_DATA(0);
		}
		NOP();
		HC595_CS(0);
		NOP();
		HC595_CS(1);
		
	}
	NOP();
	HC595_CLK(0);
	NOP();
	HC595_CLK(1);
	NOP();
	
	return 0;
}


void hc595_write(hc595_io_t io, uint8 val)
{
	uint8 j;
	
	if(val){
		hc595_value |= (1 << io); 
	}else{
		hc595_value &= ~(1 << io); 
	}
	
	SYS_TRACE("hc595_value = %2x\r\n", hc595_value);
	 
	for(j = 0; j < 8; j++){
		if(hc595_value & (0x80 >> j)){
			HC595_DATA(1);
		}else{
			HC595_DATA(0);
		}
		NOP();
		HC595_CS(0);
		NOP();
		HC595_CS(1);
	}
	NOP();
	HC595_CLK(0);
	NOP();
	HC595_CLK(1);
	NOP();

}
























