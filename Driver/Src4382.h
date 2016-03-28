#ifndef _SRC4382_H_
#define _SRC4382_H_


#define SRC4382_IIC_ADDRESS  0xe2

#define SRC4382_RST_H  GPIO_PIN_SET(SRC4382_RST_PORT, SRC4382_RST_PIN, 1)
#define SRC4382_RST_L  GPIO_PIN_SET(SRC4382_RST_PORT, SRC4382_RST_PIN, 0)

enum src4382_reg{
	POWER_DOWN_RST = 1,
	GLOBAL_INT_STATUS,
	PORT_A_CONTROL_1,
	PORT_A_CONTROL_2,
	PORT_B_CONTROL_1,
	PORT_B_CONTROL_2,
	TRANS_CONTROL_1,
	TRANS_CONTROL_2,
	TRANS_CONTROL_3,
	SRC_DIT_STATUS,
	SRC_DIT_INT_MASK,
	SRC_DIT_INT_MODE,
	RECIVER_CONTROL_1,
	RECIVER_CONTROL_2,
	RECIVER_PLL1_CONFIG_1,
	RECIVER_PLL1_CONFIG_2,
	RECIVER_PLL1_CONFIG_3,
	NON_PCM_AUDIO_DETECT,
	RECIVER_STATUS_1,
	RECIVER_STATUS_2,
	RECIVER_STATUS_3,
	RECIVER_INT_MASK_1,
	RECIVER_INT_MASK_2,
	RECIVER_INT_MODE_1,
	RECIVER_INT_MODE_2,
	RECIVER_INT_MODE_3,
	GPIO_1_CONTROL,
	GPIO_2_CONTROL,
	GPIO_3_CONTROL,
	GPIO_4_CONTROL,
	
	SRC_CONTROL_1 = 0x2D,
	SRC_CONTROL_2,
	SRC_CONTROL_3,
	SRC_CONTROL_4,
	SRC_CONTROL_5,
	
};

typedef enum{
	SRC4382_SRC_RX1 = 0,
	SRC4382_SRC_RX2,
	SRC4382_SRC_RX3,
	SRC4382_SRC_RX4,
	SRC4382_SRC_IIS_MDC,
	SRC4382_SRC_IIS_BT
}src4382_src_t;


typedef enum
{
	SRC4392_GPIO1=0,
	SRC4392_GPIO2,
	SRC4392_GPIO3,
	SRC4392_GPIO4
}src4382_gpio_t;

typedef enum
{
	SRC4392_DIV_128=0,
	SRC4392_DIV_256,
	SRC4392_DIV_384,
	SRC4392_DIV_512
}src4392_div_t;


void src4382_sda(int level);


void src4382_scl(int level);

int src4382_DIR_input_select(src4382_src_t src);

int src4382_PortA_input_select(src4382_src_t src);

int src4382_PortB_input_select(src4382_src_t src);

int src4382_PortB_setDiv(src4392_div_t div);

int src4382_DIT_input_select(src4382_src_t src);

int src4382_DIT_setDiv(src4392_div_t div);

uint8 src4382_sda_in(void);


int src4382_PortB_Mute_Control(swicth_state_t state);

int src4382_SRC_input_select(src4382_src_t src);

int src4382_init(void);

uint8 src4382_audio_detect(uint32 a, uint8 b);

void src4382_PortB_refrence_change(void);

void src4382_SRC_refrence_change(void);

#endif








