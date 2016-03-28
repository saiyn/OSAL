#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "lcd_driver.h"
#include "bsp.h"
#include "OSAL_Console.h"

#define LCD_RS(x) GPIO_PIN_SET(LCD_RS_PORT, LCD_RS_PIN, x)

#define LCD_RW(x) GPIO_PIN_SET(LCD_RW_PORT, LCD_RW_PIN, x)

#define LCD_EN(x) GPIO_PIN_SET(LCD_EN_PORT, LCD_EN_PIN, x)

#define LCD_OPERATE_TIMEOUT  10000

extern void display_init_menu(void);

static uint8 CGRAM[64] = {
	
0x01,0x03,0x07,0x0f,0x07,0x03,0x01,0x00,
0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,
0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,
0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,0x1c,

0x00,0x00,0x04,0x0e,0x1f,0x0e,0x04,0x00,
0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,0x1f,

0x10,0x18,0x1c,0x1e,0x1c,0x18,0x10,0x00,
0x00,0x00,0x00,0x0,0x04,0x0e,0x1f,0x1f
};

static void lcd_bus_write(uint8 cmd, uint8 index);

static volatile uint16 stmr;

#define LCD_SHORT_DELAY { \
	stmr = 5; \
	do{ \
	}while(stmr--); \
}

//#define LCD_SHORT_DELAY  bsp_delay_us(1)

#define LCD_LONG_DELAY { \
	stmr = 10; \
	do{ \
	}while(stmr--); \
}

//#define LCD_LONG_DELAY bsp_delay_us(10)



static void lcd_bus_write(uint8 cmd, uint8 index)
{
	uint8 temp[4] = {0};
  uint8 j; 

  if(index == 0){
		cmd = (cmd >> 4) & 0x0f;
	}else{
		cmd &= 0x0f;
	}
	
	for(j = 0; j < 4; j++){
		temp[j] = cmd & 0x01;
		cmd >>= 1;
	}
	
		GPIO_PIN_SET(LCD_DB7_PORT, LCD_DB7_PIN, temp[3]);
	  GPIO_PIN_SET(LCD_DB6_PORT, LCD_DB6_PIN, temp[2]);
		GPIO_PIN_SET(LCD_DB5_PORT, LCD_DB5_PIN, temp[1]);
		GPIO_PIN_SET(LCD_DB4_PORT, LCD_DB4_PIN, temp[0]);
	
	
//	GPIOPinWrite(LCD_DB7_PORT, LCD_DB7_PIN | LCD_DB6_PIN, (temp[3] << 4) | (temp[2] << 5));
//	GPIOPinWrite(LCD_DB5_PORT, LCD_DB5_PIN | LCD_DB4_PIN, (temp[1] << 4) | (temp[0] << 5));
}

static int lcd_check_busy(void)
{
	uint32 cnt = 0;
	int retval = 0;
	
	GPIOPinTypeGPIOInput(LCD_DB7_PORT, LCD_DB7_PIN);
	
	while(1){
		LCD_RS(0);
		LCD_RW(1);
		LCD_SHORT_DELAY;
		LCD_EN(1);
		LCD_SHORT_DELAY;
		if(GPIOPinRead(LCD_DB7_PORT, LCD_DB7_PIN) == 0){
			break;
		}else{
			cnt++;
		}
		
		if(cnt >= LCD_OPERATE_TIMEOUT){
			retval = -1;
			break;
		}
		
		LCD_EN(0);
		LCD_SHORT_DELAY;
	}
	
	GPIOPinTypeGPIOOutput(LCD_DB7_PORT, LCD_DB7_PIN);
	
	return retval;
}

int lcd_write_cmd(uint8 cmd)
{
	int retval = 0;
	uint8 i;
	
	retval = lcd_check_busy();
	if(retval < 0){
		SYS_TRACE("lcd_write_cmd timeout\r\n");
		return -1;
	}
	
	for(i = 0; i < 2; i++){
		LCD_RS(0);
		LCD_RW(0);
		LCD_SHORT_DELAY;
		LCD_EN(1);
	//	LCD_SHORT_DELAY;
		lcd_bus_write(cmd, i);
		LCD_SHORT_DELAY;
		LCD_EN(0);
		LCD_LONG_DELAY;
	}
	
	return retval;
}

int lcd_write_data(uint8 data)
{
	int retval = 0;
	uint8 i;
	
	retval = lcd_check_busy();
	if(retval < 0){
		SYS_TRACE("lcd_write_data timeout\r\n");
		return -1;
	}
	
	for(i = 0; i < 2; i++){
		LCD_RS(1);
		LCD_RW(0);
		LCD_SHORT_DELAY;
		LCD_EN(1);
		//LCD_SHORT_DELAY;
		lcd_bus_write(data, i);
		LCD_SHORT_DELAY;
		LCD_EN(0);
		LCD_LONG_DELAY;
	}
	
	return retval;
}


int lcd_init(void)
{
	uint8 i = 0;
	
	LCD_RS(0);
	LCD_RW(0);
	LCD_EN(0);
	
	lcd_write_cmd(0x28);
	
	/*clear dispaly*/
	lcd_write_cmd(0x01);
	bsp_delay_ms(4);
	
	/*set mode*/
	lcd_write_cmd(0x06);
	
	/*display on*/
	lcd_write_cmd(0x0c);
	
	/*clear dispaly*/
	lcd_write_cmd(0x01);
	bsp_delay_ms(4);
	

	/*load special character to CGRAM*/
	lcd_write_cmd(0x40);
	for(i = 0; i < 64; i++){
		lcd_write_data(CGRAM[i]);
	}

	
	display_init_menu();
	/*display on, need do this again here ? ?*/
	lcd_write_cmd(0x0c);
	
	SYS_TRACE("lcd init done\r\n");
	return 0;
}







