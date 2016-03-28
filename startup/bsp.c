#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "njw1194.h"
#include "OSAL_RingBuf.h"
#include "IR_driver.h"
#include "OSAL_Key.h"
#include "OSAL_Soft_IIC.h"
#include "EPRom_driver.h"

#define BSP_SPI_RECV_TIMEOUT  500

#define GPIO_PORTD_CR_R     (*((volatile unsigned long *)0x40007524))
#define GPIO_PORTD_LOCK_R   (*((volatile unsigned long *)0x40007520))
#define GPIO_PORTF_CR_R     (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_LOCK_R   (*((volatile unsigned long *)0x40025520))

void bsp_gpio_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOC);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOG);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOH);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOJ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOK);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOL);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOM);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPION);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOP);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOQ);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOR);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOS);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOT);
	
	GPIO_PORTF_LOCK_R = 0x4C4F434B;
	GPIO_PORTD_LOCK_R = 0x4C4F434B;

	GPIO_PORTF_CR_R = 0x1;
	GPIO_PORTD_CR_R = (1<<7);
	
	

  /*init lcd related gpio*/
	GPIOPinTypeGPIOOutput(LCD_RS_PORT, LCD_RS_PIN);
	GPIOPinTypeGPIOOutput(LCD_RW_PORT, LCD_RW_PIN);
	GPIOPinTypeGPIOOutput(LCD_EN_PORT, LCD_EN_PIN);
	GPIOPinTypeGPIOOutput(LCD_DB4_PORT, LCD_DB4_PIN);
	GPIOPinTypeGPIOOutput(LCD_DB5_PORT, LCD_DB5_PIN);
	GPIOPinTypeGPIOOutput(LCD_DB6_PORT, LCD_DB6_PIN);
  GPIOPinTypeGPIOOutput(LCD_DB7_PORT, LCD_DB7_PIN);
	GPIOPinTypeGPIOOutput(LCD_PWM_PORT, LCD_PWM_PIN);
	
	
	/*eeprom releated gpio init*/
  GPIOPinTypeGPIOOutput(EPROM_SDA_PORT, EPROM_SDA_PIN);
	GPIOPinTypeGPIOOutput(EPROM_SCL_PORT, EPROM_SCL_PIN);
	
	/*init key related gpio*/
	GPIOPinTypeGPIOOutput(KEY_STB_PORT, KEY_STB_PIN);
	GPIOPinTypeGPIOOutput(KEY_SCK_PORT, KEY_SCK_PIN);
	GPIOPinTypeGPIOInput(KEY_MISO_PORT, KEY_MISO_PIN);
	
	/*init encode related gpio*/
	GPIOPinTypeGPIOInput(ENCODE_A_PORT, ENCODE_A_PIN);
	GPIOPinTypeGPIOInput(ENCODE_B_PORT, ENCODE_B_PIN);
	GPIOIntTypeSet(ENCODE_B_PORT ,ENCODE_B_PIN , GPIO_FALLING_EDGE);
	IntEnable(ENCODE_B_INT);
	
	/*init njw1194 related gpio*/
	GPIOPinTypeGPIOOutput(NJW1194_MOSI_PORT, NJW1194_MOSI_PIN);
	GPIOPinTypeGPIOOutput(NJW1194_CS_PORT, NJW1194_CS_PIN);
	GPIOPinTypeGPIOOutput(NJW1194_CLK_PORT, NJW1194_CLK_PIN);
	NJW1194_CS(1);
	NJW1194_CLK(1);
	
	/*init src4382 related gpio*/
	GPIOPinTypeGPIOOutput(SRC4382_IIC_SDA_PORT, SRC4382_IIC_SDA_PIN);
	GPIOPinTypeGPIOOutput(SRC4382_IIC_SCL_PORT, SRC4382_IIC_SCL_PIN);
	GPIOPinTypeGPIOOutput(SRC4382_RST_PORT, SRC4382_RST_PIN);
	GPIO_PIN_SET(SRC4382_RST_PORT, SRC4382_RST_PIN, 0);
	
	GPIOPinTypeGPIOInput(SRC_READY_PORT, SRC_READY_PIN);
	
	
	/*power management gpio*/
	GPIOPinTypeGPIOOutput(AC_STANDBY_PORT, AC_STANDBY_PIN);
	GPIOPinTypeGPIOOutput(DC5V_EN_PORT, DC5V_EN_PIN);
	GPIOPinTypeGPIOOutput(DC3V_EN_PORT, DC3V_EN_PIN);

	
	/*init audio control releated gpio*/
	GPIOPinTypeGPIOOutput(AMP_MUTE_PORT, AMP_MUTE_PIN);
	GPIO_PIN_SET(AMP_MUTE_PORT, AMP_MUTE_PIN, 1);
	GPIOPinTypeGPIOOutput(AMP_MUTE2_PORT, AMP_MUTE2_PIN);
	GPIO_PIN_SET(AMP_MUTE2_PORT, AMP_MUTE2_PIN, 1);
	
	/*init 74hc595 related gpio*/
	GPIOPinTypeGPIOOutput(HC595_DATA_PORT, HC595_DATA_PIN);
	GPIOPinTypeGPIOOutput(HC595_CLK_PORT, HC595_CLK_PIN);
	GPIOPinTypeGPIOOutput(HC595_CS_PORT, HC595_CS_PIN);
	GPIO_PIN_SET(HC595_CS_PORT, HC595_CS_PIN, 1);
	
	/*usb*/
	GPIOPinTypeGPIOOutput(USB_EN_PORT, USB_EN_PIN);
	GPIO_PIN_SET(USB_EN_PORT, USB_EN_PIN, 1);
	GPIOPinTypeGPIOOutput(USB_SELECT_PORT, USB_SELECT_PIN);
	USB_SWITCH_TO_MCU;
	
	/*init led related gpio*/
	GPIOPinTypeGPIOOutput(LEDA_PORT, LEDA_PIN);
	GPIOPinTypeGPIOOutput(LEDB_PORT, LEDB_PIN);
	GPIOPinTypeGPIOOutput(LED_PWM_PORT, LED_PWM_PIN);
	GPIO_PIN_SET(LED_PWM_PORT, LED_PWM_PIN, 1);
	
	/*init MDC related gpio*/
	GPIOPinTypeGPIOOutput(MDC_CS_PORT, MDC_CS_PIN);
	GPIO_PIN_SET(MDC_CS_PORT, MDC_CS_PIN, 1);
	
	GPIOPinTypeGPIOOutput(MDC_PS01_PORT, MDC_PS01_PIN);
	GPIOPinTypeGPIOOutput(MDC_PS02_PORT, MDC_PS02_PIN);
	GPIOPinTypeGPIOOutput(MDC_M0PS_ON_PORT, MDC_M0PS_ON_PIN);
	GPIOPinTypeGPIOOutput(MDC_M1PS_ON_PORT, MDC_M1PS_ON_PIN);
	GPIOPinTypeGPIOOutput(MDC_PM0EN_PORT, MDC_PM0EN_PIN);
	GPIOPinTypeGPIOOutput(MDC_PM1EN_PORT, MDC_PM1EN_PIN);
	
//	GPIOPinTypeGPIOOutput(GPIO1M0_PORT, GPIO1M0_PIN);
//	GPIOPinTypeGPIOOutput(GPIO2M0_PORT, GPIO2M0_PIN);
//	GPIOPinTypeGPIOOutput(GPIO3M0_PORT, GPIO3M0_PIN);
//	GPIOPinTypeGPIOOutput(GPIO4M0_PORT, GPIO4M0_PIN);
//	GPIOPinTypeGPIOOutput(GPIO1M1_PORT, GPIO1M1_PIN);
//	GPIOPinTypeGPIOOutput(GPIO2M1_PORT, GPIO2M1_PIN);
//	GPIOPinTypeGPIOOutput(GPIO3M1_PORT, GPIO3M1_PIN);
//	GPIOPinTypeGPIOOutput(GPIO4M1_PORT, GPIO4M1_PIN);
	
	GPIOPinTypeGPIOOutput(MDC_PA0_PORT, MDC_PA0_PIN);
	GPIOPinTypeGPIOOutput(MDC_PA1_PORT, MDC_PA1_PIN);
	GPIOPinTypeGPIOOutput(MDC_PA2_PORT, MDC_PA2_PIN);
	
	GPIOPinTypeGPIOInput(MDC_INTM0_PORT, MDC_INTM0_PIN);
	GPIOPinTypeGPIOInput(MDC_INTM1_PORT, MDC_INTM1_PIN);
	
	GPIOPinTypeGPIOOutput(MDC_IIS_SELECT_PORT, MDC_IIS_SELECT_PIN);
	
	GPIOPinTypeGPIOOutput(MDC_M0M1USBEN_PORT, MDC_M0M1USBEN_PIN);
	GPIOPinTypeGPIOOutput(MDC_M0M1USBS_PORT, MDC_M0M1USBS_PIN);
	
	GPIOPinTypeGPIOOutput(MDC_SDA0_PORT, MDC_SDA0_PIN);
	GPIOPinTypeGPIOOutput(MDC_SCL0_PORT, MDC_SCL0_PIN);
	GPIOPinTypeGPIOOutput(MDC_SDA1_PORT, MDC_SDA1_PIN);
	GPIOPinTypeGPIOOutput(MDC_SCL1_PORT, MDC_SCL1_PIN);
	
	/*init detect gpio*/
	GPIOPinTypeGPIOInput(AUDIO_SENSE_PORT, AUDIO_SENSE_PIN);
	GPIOPinTypeGPIOInput(TRIGGER_IN_PORT, TRIGGER_IN_PIN);
	GPIOPinTypeGPIOInput(TRIGGER_LEVEL_PORT, TRIGGER_LEVEL_PIN);
	GPIOPinTypeGPIOInput(HP_INSERT_DETECT_PORT, HP_INSERT_DETECT_PIN);
	GPIOPinTypeGPIOInput(AC_DETECT_PORT, AC_DETECT_PIN);
	GPIOPinTypeGPIOInput(DC_ERROR2_PORT, DC_ERROR2_PIN);
	GPIOPinTypeGPIOInput(DC_ERROR_PORT, DC_ERROR_PIN);
	GPIOPinTypeGPIOInput(OVER_TEMP_R_PORT, OVER_TEMP_R_PIN);
	GPIOPinTypeGPIOInput(OVER_TEMP_L_PORT, OVER_TEMP_L_PIN);
  
	/*bt related gpio init*/
	GPIOPinTypeGPIOOutput(BT_VREG_PORT, BT_VREG_PIN);
	GPIOPinTypeGPIOOutput(BT_POWER_PORT, BT_POWER_PIN);
	GPIOPinTypeGPIOOutput(BT_RST_PORT, BT_RST_PIN);
  GPIO_PIN_SET(BT_RST_PORT, BT_RST_PIN, 0);
	
	/*IR related gpio init*/
	GPIOPinInputPu(FRONT_IR_PORT, FRONT_IR_PIN);
	GPIOIntTypeSet(FRONT_IR_PORT, FRONT_IR_PIN, GPIO_FALLING_EDGE);
	IntEnable(FRONT_IR_IRQ);
	GPIOIntEnable(FRONT_IR_PORT, FRONT_IR_PIN);
	
	GPIOPinInputPu(IR_IN_PORT, IR_IN_PIN);
	GPIOIntTypeSet(IR_IN_PORT, IR_IN_PIN, GPIO_FALLING_EDGE);
	IntEnable(IR_IN_IRQ);
	GPIOIntEnable(IR_IN_PORT, IR_IN_PIN);
	
}


void bsp_bt_uart_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART6);
	GPIOPinConfigure(GPIO_PP0_U6RX);
	GPIOPinConfigure(GPIO_PP1_U6TX);
	GPIOPinTypeUART(BT_UART_PORT, BT_UART_RX_PIN | BT_UART_TX_PIN);
	UARTConfigSetExpClk(UART6_BASE, gSysClock, 115200,(UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));

	//	Enable RX interrupt
	UARTIntEnable(UART6_BASE, UART_INT_RX|UART_INT_RT);

	// 	Enable UART6
	UARTEnable(UART6_BASE); 
	
}

void bsp_nad_app_uart_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART3);
	GPIOPinConfigure(GPIO_PJ0_U3RX);
	GPIOPinConfigure(GPIO_PJ1_U3TX);
	GPIOPinTypeUART(NAD_APP_UART_PORT, NAD_APP_UART_RX_PIN | NAD_APP_UART_TX_PIN);
	UARTConfigSetExpClk(UART3_BASE, gSysClock, 115200,(UART_CONFIG_WLEN_8|UART_CONFIG_STOP_ONE |UART_CONFIG_PAR_NONE));

	//	Enable RX interrupt
	UARTIntEnable(UART3_BASE, UART_INT_RX|UART_INT_RT);

	// 	Enable UART6
	UARTEnable(UART3_BASE); 

	
}




void bsp_adc_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_ADC0);
	GPIOPinTypeADC(MDC_CARD_DETECT0_PORT, MDC_CARD_DETECT0_PIN);
  GPIOPinTypeADC(MDC_CARD_DETECT1_PORT, MDC_CARD_DETECT1_PIN);
	/*disable the sample sequence before config it*/
	ADCSequenceDisable(ADC0_BASE, 0);
	ADCSequenceConfigure(ADC0_BASE, 0, ADC_TRIGGER_PROCESSOR, 0);  
	ADCHardwareOversampleConfigure(ADC0_BASE, 64);
  ADCSequenceStepConfigure(ADC0_BASE, 0, 0, ADC_CTL_CH11);	
  ADCSequenceStepConfigure(ADC0_BASE, 0, 1, ADC_CTL_CH10 | ADC_CTL_IE | ADC_CTL_END);	
	ADCSequenceEnable(ADC0_BASE, 0);
  ADCIntClear(ADC0_BASE, 0);
	
}

void bsp_timer0_init(void)
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC_UP);
	TimerLoadSet(TIMER0_BASE, TIMER_A, 0xffffffff);

	TimerEnable(TIMER0_BASE, TIMER_A);
}

uint32 bsp_timer0_get_time(void)
{
	  return TimerValueGet(TIMER0_BASE, TIMER_A)/TIME0_DIVISION;	
}


void bsp_timer1_init(void)
{
	/*timer1 1Ms*/ 
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
	//TimerLoadSet(TIMER1_BASE, TIMER_A, gSysClock/10000);   
	TimerLoadSet(TIMER1_BASE, TIMER_A, gSysClock/1000);   

	IntEnable(INT_TIMER1A);
	
	TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	
	TimerEnable(TIMER1_BASE, TIMER_A);
	
}


void Timer1IntHandler(void)
{
	static uint16 cnt = 0;
	
	TimerIntClear(TIMER1_BASE, TIMER_TIMA_TIMEOUT);

  if(cnt++ == 10){
		 KeyScanLoop();
		cnt = 0;
	}
	
	ir_timeout_check();
}

void bsp_pwm1_init(void)
{
	//SysCtlPWMClockSet(SYSCTL_PWMDIV_64);
	/*Enable device*/
  SysCtlPeripheralEnable(SYSCTL_PERIPH_PWM1);
	/*Set clock divider*/
  PWMClockSet(PWM1_BASE, PWM_SYSCLK_DIV_64);
	/*Enable PWM pin*/
  GPIOPinConfigure(LCD_PWM_CHANNEL);
  GPIOPinTypePWM(LCD_PWM_PORT, LCD_PWM_PIN);
	/*Configure PWM generator*/
  PWMGenConfigure(PWM1_BASE, PWM_GEN_0,(PWM_GEN_MODE_DOWN | PWM_GEN_MODE_NO_SYNC));
	/*Set PWM timer period*/
  PWMGenPeriodSet(PWM1_BASE, PWM_GEN_0,gSysClock/10000);
	/*Set width for PWM1*/
  PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, 50*PWMGenPeriodGet(PWM1_BASE,PWM_GEN_0)/100);
	/*Enable output*/
  PWMOutputState(PWM1_BASE, PWM_OUT_1_BIT, 1);
  /*Enable Generator*/
  PWMGenEnable(PWM1_BASE, PWM_GEN_0);
}


void bsp_lcd_bright_control(uint8 duty)
{
	 if(duty){
		  PWMOutputState(PWM1_BASE, PWM_OUT_1_BIT, 1);
		  PWMPulseWidthSet(PWM1_BASE, PWM_OUT_1, duty*PWMGenPeriodGet(PWM1_BASE,PWM_GEN_0)/100);
	 }else{
		  PWMOutputState(PWM1_BASE, PWM_OUT_1_BIT, 0);
	 }
}


void bsp_spi0_init(void)
{
	 SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);
	 /*SSI*/
	 GPIOPinConfigure(GPIO_PA2_SSI0CLK);
	 GPIOPinConfigure(GPIO_PA4_SSI0XDAT0);
	 GPIOPinConfigure(GPIO_PA5_SSI0XDAT1);
	 GPIOPinTypeSSI(MDC_MOSI_PORT, MDC_MOSI_PIN | MDC_MISO_PIN | MDC_CLK_PIN);
   /*CS */
	 GPIOPinTypeGPIOOutput(MDC_CS_PORT, MDC_CS_PIN);
	
	 SSIConfigSetExpClk(SSI0_BASE, gSysClock, SSI_FRF_MOTO_MODE_3,
	                   SSI_MODE_MASTER, 400000, 8);
	
//	SSIConfigSetExpClk(SSI0_BASE, gSysClock, SSI_FRF_MOTO_MODE_2,
//	                   SSI_MODE_MASTER, 400000, 8);
	
	
	 SSIEnable(SSI0_BASE);		
	 /*pull up the cs*/
	 GPIO_PIN_SET(MDC_CS_PORT, MDC_CS_PIN, 1);
}

void bsp_spi0_ctl(spi_ctl_t *ctl)
{
	 uint32 protocal;
	
	 SSIDisable(SSI0_BASE);	
	
	 if(ctl->protocal == 0){
		 protocal = SSI_FRF_MOTO_MODE_0;
	 }else if(ctl->protocal == 1){
		 protocal = SSI_FRF_MOTO_MODE_1;
	 }else if(ctl->protocal == 2){
		 protocal = SSI_FRF_MOTO_MODE_2;
	 }else if(ctl->protocal == 3){
		 protocal = SSI_FRF_MOTO_MODE_3;
	 }
	
	 SSIConfigSetExpClk(SSI0_BASE, gSysClock, protocal,
	                   SSI_MODE_MASTER, ctl->bitrate, ctl->datawith);
	 
	 SSIEnable(SSI0_BASE);
}

int bsp_spi_operate(uint32 base, uint32 data)
{
	uint16 i = 0;
	unsigned long dwSR;
	unsigned long dwDR;
	
	dwSR = base + SSI_O_SR;
	dwDR = base + SSI_O_DR;
	
	while ((HWREG(dwSR) & SSI_SR_TNF) == 0);
	
	HWREG(dwDR) = data;
	
	while ((HWREG(dwSR) & SSI_SR_RNE) == 0){
		if(i++ > BSP_SPI_RECV_TIMEOUT){
			SYS_TRACE("bsp_spi_operate timeout\r\n");
			return -1;
		}
	}
	
	return HWREG(dwDR);
}


void bsp_delay_us(uint32 time)
{
	SysCtlDelay(40 * time);
}


void bsp_delay_ms(uint32 time)
{
	 for(;time;time--)
		 bsp_delay_us(1000);
}

int bsp_adc0_load(uint16 *buf, uint8 channle, uint8 offset)
{
	uint32 temp[BSP_ADC0_CHANNLE_MAX] = {0};
	uint8 i;
  uint32 cnt = 0;

  OSAL_ASSERT(offset < BSP_ADC0_CHANNLE_MAX);
  
  /*start the adc to sampling*/
  ADCProcessorTrigger(ADC0_BASE, 0 | ADC_TRIGGER_SIGNAL);

  while(!ADCIntStatus(ADC0_BASE, 0, false));
  ADCIntClear(ADC0_BASE, 0);
  cnt = ADCSequenceDataGet(ADC0_BASE, 0, temp);

	for(i = 0; i < channle; i++){
		buf[i] = temp[i + offset];
		SYS_TRACE("cnt = [%d] adc channel[%d]=%d\r\n", cnt, i,  buf[i]);
	}

	return 0;
}


uint32 bsp_get_elapsedMs(uint32 start)
{
	 uint32 elapsed;
	 uint32 cur;
	 
	 cur = gTickCount;
	
	 if(cur >= start){
		 elapsed = cur - start;
	 }else{
		 elapsed = 0xffffffff - (start - cur);
	 }
	 
	 return elapsed;
}

uint32 bsp_get_curMs(void)
{
	 return gTickCount;
}


uint8 GPIO_ReadSinglePin(uint32 ulPort, uint8 ucPins)
{
	if ((HWREG(ulPort + (GPIO_O_DATA + (ucPins << 2)))) != 0)
		return 1;
	else
		return 0;
}

void GPIOPinInputPu(UINT32 ulPort, UINT8 ucPins)
{
	GPIOPinTypeGPIOInput(ulPort, ucPins);
	HWREG(ulPort + GPIO_O_PUR) |= ucPins;
}


void bsp_low_power_handler(void)
{
	uint8 buf[10] = {0};
	
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

	
	/*eeprom releated gpio init*/
  GPIOPinTypeGPIOOutput(EPROM_SDA_PORT, EPROM_SDA_PIN);
	GPIOPinTypeGPIOOutput(EPROM_SCL_PORT, EPROM_SCL_PIN);
	
	eeprom_read(EPROM, SYS_LOW_POWER_FLAG_ADDRESS, buf, strlen("LOWPOWER"));

//  if(strcmp((char *)buf, "LOWPOWER") == 0){
//		SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
//		GPIOPinTypeGPIOInput(POWER_KEY_PORT, POWER_KEY_PIN);
//	  GPIOIntTypeSet(POWER_KEY_PORT, POWER_KEY_PIN, GPIO_RISING_EDGE);
//		IntEnable(INT_KEY_POWER);
//	  GPIOIntEnable(POWER_KEY_PORT, POWER_KEY_PIN);
//		
//		eeprom_write(SYS_LOW_POWER_FLAG_ADDRESS, (uint8 *)"12345678", 8);
//		
//		bsp_delay_ms(100);
//	
//		
		SysCtlDeepSleep();
//	}
}




