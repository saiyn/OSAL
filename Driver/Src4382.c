#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Src4382.h"
#include "OSAL_Soft_IIC.h"
#include "OSAL_Console.h"

#define DIR_RX_1                0x00
#define DIR_RX_2                0x01
#define DIR_RX_3                0x02
#define DIR_RX_4                0x03

int src4382_init(void)
{
	SRC4382_RST_L;
	bsp_delay_ms(50);
	SRC4382_RST_H;
	bsp_delay_ms(50);
	
    /*power on each part, must do this in the last*/
	if(osal_iic_write_byte(SRC4382, POWER_DOWN_RST, 0x3f) < 0) return -1;
	
	/*confige port A to I2s, slave, 24bit, and disable SDOUTA*/
	if(osal_iic_write_byte(SRC4382, PORT_A_CONTROL_1, 0x41) < 0) return -1;
		
	/*confige port B to I2s, master, 24bit, and select port A as input source temporarily*/
	if(osal_iic_write_byte(SRC4382, PORT_B_CONTROL_1, 0x19) < 0) return -1;
	
	/*select MCLK for port B and divide by 256 for the LRCLK*/
	if(osal_iic_write_byte(SRC4382, PORT_B_CONTROL_2, 0x01) < 0) return -1;
	
	/*temporarily select RX1 as the input source and enable the C and U buffre transfer*/
	if(osal_iic_write_byte(SRC4382, RECIVER_CONTROL_1, 0x08) < 0) return -1;
	
	/*enable RXCKO and no stop and no auto mute*/
	if(osal_iic_write_byte(SRC4382, RECIVER_CONTROL_2, 0x11) < 0) return -1;
	
	/*p = 2, j=8, d=0*/
	if(osal_iic_write_byte(SRC4382, RECIVER_PLL1_CONFIG_1, 0x22) < 0) return -1;
	if(osal_iic_write_byte(SRC4382, RECIVER_PLL1_CONFIG_2, 0x00) < 0) return -1;
	if(osal_iic_write_byte(SRC4382, RECIVER_PLL1_CONFIG_3, 0x00) < 0) return -1;
	
	/*added for test*/
	if(osal_iic_write_byte(SRC4382, SRC_DIT_INT_MASK, 0x30) < 0) return -1;
	if(osal_iic_write_byte(SRC4382, SRC_DIT_INT_MODE, 0x30) < 0) return -1;
	
	/*enable all INT*/
	if(osal_iic_write_byte(SRC4382, RECIVER_INT_MASK_1, 0xff) < 0) return -1;
	/*keep INT status*/
	if(osal_iic_write_byte(SRC4382, RECIVER_INT_MODE_1, 0x20) < 0) return -1;
	
	if(osal_iic_write_byte(SRC4382, SRC_CONTROL_2, 0x00) < 0) return -1;
	
	
	SYS_TRACE("src4382_init done\r\n");
	return 0;
}

int src4382_DIR_input_select(src4382_src_t src)
{
  uint8 val = DIR_RX_1;

  switch(src){
	 case SRC4382_SRC_RX1:
	 case SRC4382_SRC_RX2:
	 case SRC4382_SRC_RX3:
	 case SRC4382_SRC_RX4:
		   val = src;
	  break;
	 
	default:
		break;
	}		
	
  if(osal_iic_write_byte(SRC4382, RECIVER_CONTROL_1, (val | 0x08)) < 0) return -1;
	
	return 0;
}

static int src4382_setpin(src4382_gpio_t pin, uint8 val)
{
	if(osal_iic_write_byte(SRC4382, GPIO_1_CONTROL + pin, val) < 0) return -1;
	
	return 0;
}

int src4382_PortA_input_select(src4382_src_t src)
{
	int retval = 0;
	
	if(src == SRC4382_SRC_IIS_MDC){
		retval = src4382_setpin(SRC4392_GPIO3, 0);
	}else if(src == SRC4382_SRC_IIS_BT){
		retval = src4382_setpin(SRC4392_GPIO3, 1);
	}else{
		retval = -1;
	}
	
	return retval;
}

int src4382_PortB_input_select(src4382_src_t src)
{
	int retval = 0;
	uint8 val;
	
	osal_iic_read_buf(SRC4382, PORT_B_CONTROL_2, &val, 1);
	
	SYS_TRACE("PORT_B_CONTROL_2 = %2x\r\n", val);
	
	switch(src){
		
		case SRC4382_SRC_RX1:
		case SRC4382_SRC_RX2:
		case SRC4382_SRC_RX3:
		case SRC4382_SRC_RX4:
			retval = src4382_DIR_input_select(src);
		  /*confige port B to I2s, master, 24bit, and select SRC as input source*/
		  retval = osal_iic_write_byte(SRC4382, PORT_B_CONTROL_1, 0x39);
		  /*select RXCKO as the master clock source*/
		 // val &= ~(1 << 3);
//		  retval = osal_iic_write_byte(SRC4382, PORT_B_CONTROL_2, val);
			break;
		
		case SRC4382_SRC_IIS_MDC:
		case SRC4382_SRC_IIS_BT:
			retval = src4382_PortA_input_select(src);
		  /*confige port B to I2s, master, 24bit, and select SRC as input source*/
		  retval = osal_iic_write_byte(SRC4382, PORT_B_CONTROL_1, 0x39);
//		  /*select MCLK as the master clock source*/
//		  val &= ~(1 << 2);
//		  retval = osal_iic_write_byte(SRC4382, PORT_B_CONTROL_2, val);
			break;
		
		default:
			break;
	}
	
	return retval;
}

void src4382_PortB_refrence_change(void)
{
	uint8 val;
	
	osal_iic_read_buf(SRC4382, PORT_B_CONTROL_2, &val, 1);
	
	val |= (1 << 2);
	
	osal_iic_write_byte(SRC4382, PORT_B_CONTROL_2, val);
	
}


void src4382_SRC_refrence_change(void)
{
	
	uint8 val;
	
	osal_iic_read_buf(SRC4382, SRC_CONTROL_1, &val, 1);
	
	val |= (1 << 2);
	
	osal_iic_write_byte(SRC4382, SRC_CONTROL_1, val);
	
	
}


int src4382_PortB_setDiv(src4392_div_t div)
{
	int retval = 0;
	uint8 val;
	
	osal_iic_read_buf(SRC4382, PORT_B_CONTROL_2, &val, 1);
	
	SYS_TRACE("PORT_B_CONTROL_2 = %2x\r\n", val);
	
	val = val & 0xfc; 
	val |= div; 
	
	retval = osal_iic_write_byte(SRC4382, PORT_B_CONTROL_2, val);
	
	return retval;
}

int src4382_PortB_Mute_Control(swicth_state_t state)
{
	int retval = 0;
	uint8 val;
	
	osal_iic_read_buf(SRC4382, PORT_B_CONTROL_1, &val, 1);
	
	SYS_TRACE("PORT_B_CONTROL_1 = %2x\r\n", val);
	
	if(OFF == state){
		val &= ~(1 << 6); 
	}else{
		val |= (1 << 6); 
	}
	
	retval = osal_iic_write_byte(SRC4382, PORT_B_CONTROL_1, val);
	
	return retval;
	
}

int src4382_DIT_input_select(src4382_src_t src)
{
	int retval = 0;
	uint8 val;
	
	osal_iic_read_buf(SRC4382, TRANS_CONTROL_1, &val, 1);
	
	SYS_TRACE("TRANS_CONTROL_1 = %2x\r\n", val);
	
	switch(src){
		
		case SRC4382_SRC_RX1:
		case SRC4382_SRC_RX2:
		case SRC4382_SRC_RX3:
		case SRC4382_SRC_RX4:
			retval = src4382_DIR_input_select(src);
		  /*selct DIR as input and select RXCKO as the master clock source*/
		  val = val & 0xe7;
		  val |= (2 << 3);
		  val |= 0x01;
		  retval = osal_iic_write_byte(SRC4382, TRANS_CONTROL_1, val);
			break;
		
		case SRC4382_SRC_IIS_MDC:
		case SRC4382_SRC_IIS_BT:
			retval = src4382_PortA_input_select(src);
		  /*selct PortA as input and select MCLK as the master clock source*/
		  val = val & 0xe7;
		  val |= (0 << 3);
		  val = val & 0x01;
		  retval = osal_iic_write_byte(SRC4382, TRANS_CONTROL_1, val);
			break;
		
		default:
			break;
	}
	
	return retval; 
}

int src4382_DIT_setDiv(src4392_div_t div)
{
  int retval = 0;
	uint8 val;
	
	osal_iic_read_buf(SRC4382, TRANS_CONTROL_1, &val, 1);
	
	SYS_TRACE("TRANS_CONTROL_1 = %2x\r\n", val);
	
	val = val & 0xf1;
	val |= (div << 1);
	
	retval = osal_iic_write_byte(SRC4382, TRANS_CONTROL_1, val);
	
	
	return retval;
}

int src4382_SRC_input_select(src4382_src_t src)
{
	int retval = 0;

	
	switch(src){
		
		case SRC4382_SRC_RX1:
		case SRC4382_SRC_RX2:
		case SRC4382_SRC_RX3:
		case SRC4382_SRC_RX4:
			retval = src4382_DIR_input_select(src);
		  /*selct DIR as input and select RXCKO as the master clock source*/
		  retval = osal_iic_write_byte(SRC4382, SRC_CONTROL_1, 0x4a);
		  
			break;
		
		case SRC4382_SRC_IIS_MDC:
		case SRC4382_SRC_IIS_BT:
			retval = src4382_PortA_input_select(src);
		  /*selct PortA as input and select MCLK as the master clock source*/
		  retval = osal_iic_write_byte(SRC4382, SRC_CONTROL_1, 0x40);
			break;
		
		default:
			break;
	}
	
	return retval; 
}

uint8 src4382_audio_detect(uint32 a, uint8 b)
{
	uint8 val;
	int retval = 0;
	
	retval = osal_iic_read_buf(SRC4382, RECIVER_STATUS_2, &val, 1);
	if(retval < 0) return 0;
	
	//SYS_TRACE("RECIVER_STATUS_2:%2x\r\n", val);
	if(val & (1 << 2)) return 0;
	else return 1;
	return 0;
}




void src4382_sda(int level)
{
	if(level){
		GPIOPinTypeGPIOInput(SRC4382_IIC_SDA_PORT, SRC4382_IIC_SDA_PIN);
		GPIO_PIN_SET(SRC4382_IIC_SDA_PORT, SRC4382_IIC_SDA_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(SRC4382_IIC_SDA_PORT, SRC4382_IIC_SDA_PIN);
		GPIO_PIN_SET(SRC4382_IIC_SDA_PORT, SRC4382_IIC_SDA_PIN, 0);
	}
}

void src4382_scl(int level)
{
	if(level){
		GPIOPinTypeGPIOInput(SRC4382_IIC_SCL_PORT, SRC4382_IIC_SCL_PIN);
		GPIO_PIN_SET(SRC4382_IIC_SCL_PORT, SRC4382_IIC_SCL_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(SRC4382_IIC_SCL_PORT, SRC4382_IIC_SCL_PIN);
		GPIO_PIN_SET(SRC4382_IIC_SCL_PORT, SRC4382_IIC_SCL_PIN, 0);
	}  
}

uint8 src4382_sda_in(void)
{
		return (GPIO_ReadSinglePin(SRC4382_IIC_SDA_PORT, SRC4382_IIC_SDA_PIN));
}



int audio(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  
  src4382_audio_detect(0,0);
		
	SYS_TRACE("audio detect done\r\n", atoi(argv[1]));
	return 0;
}

OSAL_CMD(aud, 2, audio, "audio detect test");

int src4382_test(struct cmd_tbl_s *cmdtp, int argc, char * const argv[])
{
  uint8 val = 0;
	
	osal_iic_read_buf(SRC4382, SRC_DIT_STATUS, &val, 1);
	
	SYS_TRACE("SRC_DIT_STATUS = %2x\r\n", val);
	
	osal_iic_read_buf(SRC4382, GLOBAL_INT_STATUS, &val, 1);
	
	SYS_TRACE("GLOBAL_INT_STATUS = %2x\r\n", val);
	
	osal_iic_read_buf(SRC4382, 0x32, &val, 1);
	
	SYS_TRACE("SRC RATIO1 = %2x\r\n", val);
	
	osal_iic_read_buf(SRC4382, 0x33, &val, 1);
	
	SYS_TRACE("SRC RATIO2 = %2x\r\n", val);
	
	
	return 0;
}

OSAL_CMD(srct, 2, src4382_test, "src4382 test");





