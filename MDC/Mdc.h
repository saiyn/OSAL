#ifndef _MDC_H_
#define _MDC_H_

#define MAX_INPUT_NAME_LEN 16

#define TEST_CARD_I2C_ADDR     0xA0


#define MDC_SPI_CS(x) GPIO_PIN_SET(MDC_CS_PORT, MDC_CS_PIN, x)

#define MDC_SPI_PUTC(x)  bsp_spi_operate(MDC_SPI, x)

#define MDC_PS01_H             GPIO_PIN_SET(MDC_PS01_PORT, MDC_PS01_PIN, 1)
#define MDC_PS01_L             GPIO_PIN_SET(MDC_PS01_PORT, MDC_PS01_PIN, 0)
#define MDC_PS02_H             GPIO_PIN_SET(MDC_PS02_PORT, MDC_PS02_PIN, 1) 
#define MDC_PS02_L             GPIO_PIN_SET(MDC_PS02_PORT, MDC_PS02_PIN, 0) 

#define MDC_M0PS_ON_H          GPIO_PIN_SET(MDC_M0PS_ON_PORT, MDC_M0PS_ON_PIN, 1) 
#define MDC_M0PS_ON_L          GPIO_PIN_SET(MDC_M0PS_ON_PORT, MDC_M0PS_ON_PIN, 0) 
#define MDC_M1PS_ON_H          GPIO_PIN_SET(MDC_M1PS_ON_PORT, MDC_M1PS_ON_PIN, 1) 
#define MDC_M1PS_ON_L          GPIO_PIN_SET(MDC_M1PS_ON_PORT, MDC_M1PS_ON_PIN, 0)

#define MDC_PM0EN_H            GPIO_PIN_SET(MDC_PM0EN_PORT, MDC_PM0EN_PIN, 1) 
#define MDC_PM0EN_L            GPIO_PIN_SET(MDC_PM0EN_PORT, MDC_PM0EN_PIN, 0) 
#define MDC_PM1EN_H            GPIO_PIN_SET(MDC_PM1EN_PORT, MDC_PM1EN_PIN, 1) 
#define MDC_PM1EN_L            GPIO_PIN_SET(MDC_PM1EN_PORT, MDC_PM1EN_PIN, 0) 

#define MDC_IIS_H              GPIO_PIN_SET(MDC_IIS_SELECT_PORT, MDC_IIS_SELECT_PIN, 1) 
#define MDC_IIS_L              GPIO_PIN_SET(MDC_IIS_SELECT_PORT, MDC_IIS_SELECT_PIN, 0) 

#define MDC_PA0_H              GPIO_PIN_SET(MDC_PA0_PORT, MDC_PA0_PIN, 1)
#define MDC_PA0_L              GPIO_PIN_SET(MDC_PA0_PORT, MDC_PA0_PIN, 0)
#define MDC_PA1_H              GPIO_PIN_SET(MDC_PA1_PORT, MDC_PA1_PIN, 1)
#define MDC_PA1_L              GPIO_PIN_SET(MDC_PA1_PORT, MDC_PA1_PIN, 0)
#define MDC_PA2_H              GPIO_PIN_SET(MDC_PA2_PORT, MDC_PA2_PIN, 1)
#define MDC_PA2_L              GPIO_PIN_SET(MDC_PA2_PORT, MDC_PA2_PIN, 0)





typedef enum{
	SLOT_1=0,
	SLOT_2,
	SLOT_NUM
}mdc_slot_t;

typedef struct
{
    UINT8 major;
    UINT8 minor;
    UINT8 beta;
}version_t;


typedef enum{
	MDC_DIO=0,
  MDC_USB,
  MDC_HDMI,
  MDC_ANA,
  MDC_BLUOS,
  MDC_HDMI_v2,
	MDC_CARD_NUM
}mdc_type_t;

extern char *CardTypeName[MDC_CARD_NUM + 1];

typedef enum{
	TEST_START=0,
	TEST_IN_PROCESS,
	TEST_WAIT_FOR_START_CONFIRM,
	TEST_WAIT_FOR_RESTART_CONFIRM,
}mdc_test_task_state_t;

typedef enum{
	MDC_TEST_NOT_JUDGED = 0,
	MDC_TEST_FAIL,
	MDC_TEST_PASS,
	MDC_TEST_WAIT,
	MDC_TEST_NUM
}result_t;

typedef enum{
	TEST_ITEM_I2C=0,
	TEST_ITEM_GPIO,
	TEST_ITEM_SPI,
	TEST_ITEM_OPT,
	TEST_ITEM_COAX,
	TEST_ITEM_USB,
	TEST_ITEM_VOLTAGE,
	TEST_ITEM_CURRENT,
	TEST_ITEM_CARD_READ_BACK,
	TEST_ITEM_REPORT_RESULT,
	TEST_ITEM_NUM
}test_item_t;

typedef struct{
	uint8 start_index;
	uint8 cur_index;
	uint8 num;
	test_item_t item;
	result_t report[SLOT_NUM];
	uint8 bitmap_insert;
}mdc_test_t;

typedef enum{
	MDC_GPIO_1=0,
	MDC_GPIO_2,
	MDC_GPIO_3,
	MDC_GPIO_4,
	MDC_GPIO_NUM
}mdc_gpio_t;

typedef enum
{
    TEST_INPUT_OPT=0,
    TEST_INPUT_COAX,
    TEST_INPUT_USB,
    TEST_INPUT_PWR,
    TEST_INPUT_N
}test_input_t;

typedef struct
{
    mdc_type_t           type;
    UINT8                max_input;  // the last source id in this mdc slot. eg: slot{0,1,2,n}, then the max_input = 2.
    UINT8                index;         // the same card's index. eg: 2 usb card, then the second usb card's index will be 1.
    UINT8                input;
    version_t            ver;
    char                 * const *str_cmd;
	  char  (*name)[][MAX_INPUT_NAME_LEN];
}mdc_slot_state_t;

#include "Mdc_dio.h"
#include "Mdc_usb.h"
#include "Mdc_hdmi.h"
#include "Mdc_ana.h"
#include "Mdc_bt.h"
#include "Mdc_hdmi_v2.h"

typedef struct
{
    mdc_type_t           type;
    UINT8                index;
    union
    {
        dio_setup_t       dio;
        usb_setup_t       usb;
        hdmi_setup_t      hdmi;
        hdmi_v2_setup_t   hdmi2;
        ana_setup_t       ana;
        blu_setup_t       blu;
    }card;
}mdc_setup_t;

typedef struct{
  /*index of current active slot*/
	uint8 mdc_slot_index;
	/*index of input source in current active slot*/
	uint8 mdc_slot_src_index;
	/*record each slot previous state*/
	mdc_slot_state_t prev_slot[SLOT_NUM];
	/*pointer of the instance of all the MDC cards inserted into the unit*/
	mdc_setup_t setup[SLOT_NUM];

	uint8 end_of_nv;
	
	uint8  slot_src_idx[SLOT_NUM];
	uint16 adc[SLOT_NUM];
	uint8  sources;
	/*record each slot current state*/
	mdc_slot_state_t slot[SLOT_NUM];
}mdc_t;


#define SIZEOF_MDC_NV   ((uint8 *)&(gMdc->end_of_nv) - (uint8 *)&(gMdc->mdc_slot_index))

extern mdc_t *gMdc;

int mdc_init(void);

void mdc_bus_select(mdc_slot_t slot);

void mdc_power_set(mdc_slot_t slot, uint8 state);

void mdc_pa_set(uint8 val);

void mdc_pa_set(uint8 val);

uint8 mdc_int_get(mdc_slot_t slot);

void mdc_iis_select(mdc_slot_t slot);

void mdc_usb_bus_select(mdc_slot_t slot);

void mdc_test_task_handler(sys_state_t *sys, bool user_msg, uint8 msg);

void mdc_usb_disable(mdc_slot_t slot);

void mdc_slot1_sda(int level);

void mdc_slot2_sda(int level);

void mdc_slot1_scl(int level);

void mdc_slot2_scl(int level);

uint8 mdc_slot1_sda_in(void);

uint8 mdc_slot2_sda_in(void);

#endif






