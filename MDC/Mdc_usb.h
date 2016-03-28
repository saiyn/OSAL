#ifndef _MDC_USB_H_
#define _MDC_USB_H_


typedef enum
{
    USB_BL_MSG_END=0,
    USB_BL_MSG_START,
    USB_BL_MSG_QUERY,
    USB_BL_MSG_NEXT_BLOCK,
    USB_BL_MSG_BLOCK,
    USB_BL_MSG_FLASH_ERASE,
    USB_BL_MSG_FLASH_CHECK_READY,
    USB_BL_MSG_FLASH_READY,
    USB_BL_MSG_FLASH_BUSY,
    USB_BL_MSG_GET_VERSION,
    USB_BL_MSG_FINISH,
    USB_BL_MSG_FLASH_STATE,
    USB_BL_MSG_FLASH_OK,
    USB_BL_MSG_FLASH_BAD,
    USB_BL_MSG_RUN_APP
}usb_bl_msg_t;

typedef enum
{
    USB_INPUT_PC=0,
    USB_INPUT_BACK,
    USB_INPUT_FRONT,
    USB_INPUT_N
}usb_input_t;


enum {
    BRDG_HOST_UNSET=0,
    BRDG_HOST_C390,
    BRDG_HOST_M12,
};

typedef enum {
    USB_MSG_END = 0,
    USB_MSG_START,
    USB_MSG_QUERY,  //USB_MSG_HOST
    USB_MSG_RS232,
    /*commands*/
    USB_MSG_UP,
    USB_MSG_DOWN,
    USB_MSG_LEFT,
    USB_MSG_RIGHT,
    USB_MSG_NEXT,
    USB_MSG_PREV,
    USB_MSG_EN_FFW,
    USB_MSG_DIS_FFW,
    USB_MSG_EN_FRW,
    USB_MSG_DIS_FRW,
    USB_MSG_PLAY,
    USB_MSG_PAUSE,
    USB_MSG_ENTER,
    USB_MSG_SHUFFLE,
    USB_MSG_REPEAT,
    USB_MSG_DISPLAY,
    USB_MSG_USB_FRONT,
    USB_MSG_USB_BACK,
    USB_MSG_AVR32,
    USB_MSG_TAS1020,
    USB_MSG_STOP,
    USB_MSG_FID,
    USB_MSG_RESET_NAV,
    USB_MSG_TOTAL
}usb_card_msg_t;

typedef struct
{
  UINT8   volume_control[USB_INPUT_N];
  UINT8   volume_fixed[USB_INPUT_N];
	bool		enable[USB_INPUT_N];
	char 		name[USB_INPUT_N][MAX_INPUT_NAME_LEN];
}usb_setup_t;

extern char * const DEF_USB_CMD[SLOT_NUM][USB_INPUT_N];


extern char * const DEF_USB_NAME[2][USB_INPUT_N];

void mdc_usb_input_select(mdc_slot_t slot, usb_input_t in, uint8 src_id);

int mdc_usb_init(mdc_slot_t slot, uint8 src_id);

void mdc_usb_task(mdc_slot_t slot);

void mdc_usb_send_msg(mdc_slot_t slot, char *fmt, ...);

int mdc_usb_erase_flash(mdc_slot_t slot);

void mdc_usb_finish_program(mdc_slot_t slot, uint16 crc);

int mdc_usb_send_block(mdc_slot_t slot, DWORD_VAL addr, uint8 *block, uint16 len, uint16 *crc);

int usb_bl_check_flash_status(mdc_slot_t slot);

#endif

