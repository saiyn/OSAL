#ifndef  _MDC_HDMI_V2_H_
#define  _MDC_HDMI_V2_H_



typedef enum
{
    HDMI_V2_INPUT_1=0,
    HDMI_V2_INPUT_2,
    HDMI_V2_INPUT_3,
    HDMI_V2_INPUT_TV,
    HDMI_V2_INPUT_N
}hdmi_v2_input_t;

typedef struct
{
	UINT8		volume_control[HDMI_V2_INPUT_N];
	UINT8		volume_fixed[HDMI_V2_INPUT_N];
	bool		enable[HDMI_V2_INPUT_N];
	char 		name[HDMI_V2_INPUT_N][MAX_INPUT_NAME_LEN];
}hdmi_v2_setup_t;


extern char * const DEF_HDMI_V2_CMD[SLOT_NUM][HDMI_V2_INPUT_N];

extern char * const DEF_HDMI_V2_NAME[2][HDMI_V2_INPUT_N];

void hdmi_V2_input_select(mdc_slot_t slot, hdmi_v2_input_t input);


void hdmi_V2_init(mdc_slot_t slot);

void hdmi_v2_task(mdc_slot_t slot);

void hdmi_V2_version_get(mdc_slot_t slot, version_t *ver);

void hdmi_v2_send_msg(mdc_slot_t slot, HDMI2_cmd_t cmd, uint8 *data, uint8 len);

#endif



