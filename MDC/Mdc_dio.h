#ifndef _MDC_DIO_H_
#define _MDC_DIO_H_



typedef enum
{
    DIO_INPUT_COAX1=0,
    DIO_INPUT_COAX2,
    DIO_INPUT_OPT1,
    DIO_INPUT_OPT2,
    DIO_INPUT_AESEBU,
    DIO_INPUT_N
}dio_input_t;

typedef struct
{
    UINT8        volume_control[DIO_INPUT_N];
    UINT8        volume_fixed[DIO_INPUT_N];
    bool         enable[DIO_INPUT_N];
    char         name[DIO_INPUT_N][MAX_INPUT_NAME_LEN];
}dio_setup_t;


extern char * const DEF_DIO_CMD[SLOT_NUM][DIO_INPUT_N];

extern char * const DEF_DIO_NAME[2][DIO_INPUT_N];


void mdc_dio_init(mdc_slot_t slot);

uint8 AK4118_read(uint8 reg);

int AK4118_write(uint8 reg, uint8 val);

void mdc_dio_input_select(mdc_slot_t slot, dio_input_t in);

#endif






