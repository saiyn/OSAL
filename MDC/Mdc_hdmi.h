#ifndef  _MDC_HDMI_H_
#define  _MDC_HDMI_H_



typedef enum
{
    HDMI_INPUT_1=0,
    HDMI_INPUT_2,
    HDMI_INPUT_3,
    HDMI_INPUT_N
}hdmi_input_t;

typedef struct
{
	UINT8		volume_control[HDMI_INPUT_N];
	UINT8		volume_fixed[HDMI_INPUT_N];
	bool		enable[HDMI_INPUT_N];
	char 		name[HDMI_INPUT_N][MAX_INPUT_NAME_LEN];
}hdmi_setup_t;

extern char * const DEF_HDMI_CMD[SLOT_NUM][HDMI_INPUT_N];


extern char * const DEF_HDMI_NAME[2][HDMI_INPUT_N];


void hdmi_init(mdc_slot_t slot);

void hdmi_version_get(mdc_slot_t slot, version_t *ver);

void hdmi_input_select(mdc_slot_t slot, hdmi_input_t input);

#endif







