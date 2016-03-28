#include "sys_head.h"
#include "OSAL_Task.h"
#include "OSAL.h"
#include "OSAL_Timer.h"
#include "serial.h"
#include "sysctl.h"
#include "MDC.h"
#include "System_Task.h"
#include "Soft_Timer_Task.h"
#include "BT_Task.h"
#include "NAD_Uart_App_Task.h"

#define FAULT_SYSTICK           15          // System Tick

const char *version = "V0.1.12";

uint32 gSysClock;
uint32 gTickCount;
static void hal_init(void);
static void print_version(const char *);

int main(void)
{

	hal_init();
	print_version(version);
	

	osal_init_system();
	if(gSystem_t->mdc_test){
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 20000);
	}
	osal_start_reload_timer(gSTTID, SERIAL_POLL_MSG, 500);
	osal_start_reload_timer(gBT_TaskID, BT_POLL_MSG, 150);
	osal_start_reload_timer(gNADUartID, NAD_UART_POLL_MSG, 200);
	osal_start_timerEx(gSTTID, IR_IN_POLARITY_CHECK_MSG, 150);
  while(1)
  { 
		 osal_start_system();
  }
	 
}


static void hal_init(void)
{
	 gSysClock = SysCtlClockFreqSet((SYSCTL_XTAL_25MHZ | SYSCTL_OSC_MAIN | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_480), 120000000); 
	 //gSysClock = SysCtlClockFreqSet((SYSCTL_OSC_INT | SYSCTL_USE_PLL | SYSCTL_CFG_VCO_320), 120000000);
	
	 //SysCtlDeepSleep();
	
	 /*systick = 1ms*/
	 SysTickPeriodSet(gSysClock / 1000);
   SysTickEnable();
   SysTickIntEnable();
	
	 bsp_gpio_init();
	 uart_hal_init();
	
	 bsp_adc_init();
	 bsp_spi0_init();
	 bsp_timer0_init();
	 bsp_timer1_init();
	 bsp_bt_uart_init();
	 bsp_nad_app_uart_init();
   
	 //bsp_pwm1_init();
	 IntPrioritySet(FAULT_SYSTICK, SYSTICK_INT_PRIORITY);
	 IntMasterEnable();
}

static void print_version(const char *version)
{
	s_printf("\r\n***************************************************\r\n");
#ifdef NAD_C378
	s_printf("*      NAD C378\r\n");
#else
	s_printf("*      NAD C358\r\n");
#endif
	s_printf("*      version:%s\r\n",version);
	s_printf("*      Created: %s %s \r\n", __DATE__, __TIME__);
	s_printf("***************************************************\r\n");
	
}


