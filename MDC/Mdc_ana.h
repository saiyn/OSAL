#ifndef  _MDC_ANA_H_
#define  _MDC_ANA_H_



typedef enum
{
  ANA_INPUT_PHONO=0,
	ANA_INPUT_SINGLE,
	ANA_INPUT_BALANCE,
	ANA_INPUT_N
}ana_input_t;

typedef enum
{
    PHONO_MM=0,
    PHONO_MC,
    PHONO_N
}phono_mode_t;


typedef struct
{
    UINT8               phono_mode;
    char                gain[ANA_INPUT_N];
    UINT8               adc_sr[ANA_INPUT_N];
    UINT8               volume_control[ANA_INPUT_N];
    UINT8               volume_fixed[ANA_INPUT_N];
    bool                enable[ANA_INPUT_N];
    char                name[ANA_INPUT_N][MAX_INPUT_NAME_LEN];
}ana_setup_t;

extern char * const DEF_ANA_CMD[SLOT_NUM][ANA_INPUT_N];

extern char * const DEF_ANA_NAME[2][ANA_INPUT_N];

void ana_set_power(mdc_slot_t slot);

void mdc_ana_input_select(mdc_slot_t slot, ana_input_t in);

void mdc_ana_mode_set(mdc_slot_t slot, phono_mode_t mode);

void mdc_ana_sample_set(mdc_slot_t slot, uint8 rate);

void mdc_ana_trimLevel_set(mdc_slot_t slot, ana_input_t in, int level);

void ana_init(mdc_slot_t slot);
#endif




