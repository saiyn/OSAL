#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "pcm1795.h"
#include "OSAL_Soft_IIC.h"
#include "OSAL_Console.h"


int pcm1795_init(void)
{
	PCM1795_RST_L;
	bsp_delay_ms(100);
	PCM1795_RST_H;
	bsp_delay_ms(100);
	
	/*set sck to 64fs, stereo mode*/
	if(osal_iic_write_byte(PCM1795, PCM1795_REG_20, 0x00) < 0) return -1;
	
	SYS_TRACE("pcm1795_init done\r\n");
	return 0;
}


void pcm1795_sda(int level)
{
	if(level){
		GPIOPinTypeGPIOInput(PCM1795_IIC_SDA_PORT, PCM_1795_IIC_SDA_PIN);
		GPIO_PIN_SET(PCM1795_IIC_SDA_PORT, PCM_1795_IIC_SDA_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(PCM1795_IIC_SDA_PORT, PCM_1795_IIC_SDA_PIN);
		GPIO_PIN_SET(PCM1795_IIC_SDA_PORT, PCM_1795_IIC_SDA_PIN, 0);
	}
}

void pcm1795_scl(int level)
{
	 if(level){
		GPIOPinTypeGPIOInput(PCM1795_IIC_SCL_PORT, PCM_1795_IIC_SCL_PIN);
		GPIO_PIN_SET(PCM1795_IIC_SCL_PORT, PCM_1795_IIC_SCL_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(PCM1795_IIC_SCL_PORT, PCM_1795_IIC_SCL_PIN);
		GPIO_PIN_SET(PCM1795_IIC_SCL_PORT, PCM_1795_IIC_SCL_PIN, 0);
	}  
}

uint8 pcm1795_sda_in(void)
{ 
	 return (GPIO_ReadSinglePin(PCM1795_IIC_SDA_PORT, PCM_1795_IIC_SDA_PIN));
}




int pcm_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
	
 
	pcm1795_init();

	return 0;
}

OSAL_CMD(pcm, 2, pcm_test, "pcm init test");









