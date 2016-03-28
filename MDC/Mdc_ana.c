#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Mdc.h"
#include "Mdc_ana.h"



char * const DEF_ANA_CMD[SLOT_NUM][ANA_INPUT_N]=
{
    {"Phono",  "SingleEnded",  "Balanced"},
    {"Phono",  "SingleEnded",  "Balanced"}
};

char * const DEF_ANA_NAME[2][ANA_INPUT_N]=
{
    {"Phono",   "SingleEnded",   "Balanced"},
    {"Phono 1 ", "SingleEnded1 ",  "Balanced 1 "}
};

static uint8 Buff595[SLOT_NUM];

/*--- 74HC595 ---*/
#define O_ADRST                (1<<0)
#define O_ADFS1                (1<<1)
#define O_ADFS2                (1<<2)
#define O_PSO                (1<<3)
#define O_G12                (1<<4)
#define O_MCMM                (1<<5)
#define O_12EN                (1<<6)

#define PO_ADFS1_H            Buff595[slot] |= O_ADFS1
#define PO_ADFS1_L            Buff595[slot] &= ~O_ADFS1
#define PO_ADFS2_H            Buff595[slot] |= O_ADFS2
#define PO_ADFS2_L            Buff595[slot] &= ~O_ADFS2
#define PO_ADRST_H            Buff595[slot] |= O_ADRST
#define PO_ADRST_L            Buff595[slot] &= ~O_ADRST
#define PO_G12_H            Buff595[slot] |= O_G12
#define PO_G12_L            Buff595[slot] &= ~O_G12
#define PO_PSO_H            Buff595[slot] |= O_PSO
#define PO_PSO_L            Buff595[slot] &= ~O_PSO
#define PO_MCMM_H            Buff595[slot] |= O_MCMM
#define PO_MCMM_L            Buff595[slot] &= ~O_MCMM
#define PO_12EN_H            Buff595[slot] |= O_12EN
#define PO_12EN_L            Buff595[slot] &= ~O_12EN

/*--- pcm4202 ---*/
#define PCM4202_512FS()        PO_ADFS1_L;PO_ADFS2_L
#define PCM4202_256FS()        PO_ADFS1_L;PO_ADFS2_H
#define PCM4202_128FS()        PO_ADFS1_H;PO_ADFS2_H
#define PCM4202_RESET_ON()    PO_ADRST_L
#define PCM4202_RESET_OFF()    PO_ADRST_H

/*--- njw1195 ---*/
#define NJW1195_CHIP_ADDR    1
#define NJW1195_REG_VOLA    (0<<4)
#define NJW1195_REG_VOLB    (1<<4)
#define NJW1195_REG_INPUTA    (4<<4)
#define NJW1195_REG_INPUTB    (5<<4)

#define NJW1195_PA            0
#define HC595_PA            1


static void njw1195_write(uint16 out)
{
	spi_ctl_t ctl ={16, 0, 40000};
	
	bsp_spi0_ctl(&ctl);
	
	MDC_SPI_CS(0);
	bsp_delay_us(10);
	MDC_SPI_PUTC(out);
  bsp_delay_us(10);
	MDC_SPI_CS(1);
}

static void njw1195_input_set(uint8 in)
{
	mdc_pa_set(NJW1195_PA);
	if(in == 0){
		njw1195_write((2 << 13) | (1 << 10) | NJW1195_REG_INPUTA |
            NJW1195_CHIP_ADDR);
		njw1195_write((2 << 13) | (1 << 10) | NJW1195_REG_INPUTB |
            NJW1195_CHIP_ADDR);
	}else if(in == 1){
		njw1195_write((3 << 13) | (4 << 10) | NJW1195_REG_INPUTA |
			NJW1195_CHIP_ADDR);
		njw1195_write((3 << 13) | (4 << 10) | NJW1195_REG_INPUTB |
            NJW1195_CHIP_ADDR);
	}
}


static void njw1195_volume_set(int vol)
{
#define GAIN_0DB   0x40
	
	int val;
	
	if(vol == 0xff){
		val = 0;
		SYS_TRACE("NJW1195: volume mute\r\n");
	}else{
		if(vol < -190) vol = -190;
		if(vol > 63) vol = 63;
		
		val = GAIN_0DB - vol;
		val <<= 8;
	}
	mdc_pa_set(NJW1195_PA);
	njw1195_write(val | NJW1195_REG_VOLA | NJW1195_CHIP_ADDR);
	njw1195_write(val | NJW1195_REG_VOLB | NJW1195_CHIP_ADDR);
}

static void HC595_Update(mdc_slot_t slot)
{
	spi_ctl_t ctl ={8, 0, 400000};
	
	mdc_pa_set(HC595_PA);
	
	bsp_spi0_ctl(&ctl);
	
  MDC_SPI_PUTC(Buff595[slot]);
	MDC_SPI_CS(0);
	MDC_SPI_CS(1);

}

void ana_set_power(mdc_slot_t slot)
{
	OSAL_ASSERT(slot < SLOT_NUM);
	
	 mdc_bus_select(slot);
	 Buff595[slot] = 0xFF;
	 bsp_delay_ms(10);
	
	 if(gMdc->setup[slot].card.ana.phono_mode == PHONO_MC){
		 PO_G12_H;
     PO_MCMM_H;
	 }else{
		 PO_G12_L;
     PO_MCMM_L;
	 }
	 
	 PO_PSO_L;
	 
	 HC595_Update(slot);
	 bsp_delay_ms(10);
	 mdc_bus_select(SLOT_NUM);
}


void ana_init(mdc_slot_t slot)
{
	SYS_TRACE("mdc slot[%d] Analog Card init\r\n", slot);
	
	mdc_bus_select(slot);
	njw1195_input_set(1);
	njw1195_volume_set(0);
	Buff595[slot] = 0xFF;
	bsp_delay_ms(10);
	PCM4202_RESET_OFF();
	bsp_delay_ms(10);
	PCM4202_512FS();
	bsp_delay_ms(10);
	PO_PSO_L;
	HC595_Update(slot);
	bsp_delay_ms(10);
	mdc_bus_select(SLOT_NUM);
	//bsp_delay_ms(10);
}


void mdc_ana_input_select(mdc_slot_t slot, ana_input_t in)
{
	bsp_delay_ms(10);
	
	mdc_bus_select(slot);
	
	switch(in){
		case ANA_INPUT_SINGLE:
			PO_PSO_L;
		  HC595_Update(slot);
		  njw1195_input_set(0);
			break;
		
		case ANA_INPUT_BALANCE:
			PO_PSO_L;
		  njw1195_input_set(1);
			break;
		
		case ANA_INPUT_PHONO:
			PO_PSO_H;
		  HC595_Update(slot);
		  njw1195_input_set(0);
			break;
		
		default:
			break;
	}
	
	bsp_delay_ms(15);
	
	mdc_bus_select(SLOT_NUM);
	
}


void mdc_ana_sample_set(mdc_slot_t slot, uint8 rate)
{
	bsp_delay_ms(10);
	
	mdc_bus_select(slot);
	
	switch(rate){
		case 48:
			PCM4202_512FS();
			break;
		case 96:
			PCM4202_256FS();
			break;
		case 192:
			PCM4202_128FS();
			break;
		
		default:
			break;
	}
	
	bsp_delay_ms(10);
	
	PCM4202_RESET_ON();
	HC595_Update(slot);
	bsp_delay_us(50);
	PCM4202_RESET_OFF();
	HC595_Update(slot);
	
	mdc_bus_select(slot);
}

void mdc_ana_trimLevel_set(mdc_slot_t slot, ana_input_t in, int level)
{
#define BASE_VOL  8
	
	int val;
	
	bsp_delay_ms(10);
	mdc_bus_select(slot);
	
	
	switch(in){
		
			case ANA_INPUT_SINGLE:
				val = level << 1;
			break;
		
		case ANA_INPUT_BALANCE:
			val = (level << 1) + 12;
			break;
		
		case ANA_INPUT_PHONO:
			val = level << 1;
			break;
		
		default:
			break;
	}
	
	njw1195_volume_set(val + BASE_VOL);
	
	mdc_bus_select(SLOT_NUM);
	
}

void mdc_ana_mode_set(mdc_slot_t slot, phono_mode_t mode)
{
	mdc_bus_select(slot);
	
	switch(mode){
		
		case PHONO_MC:
			PO_G12_H;
		  PO_MCMM_H;
			break;
		
		case PHONO_MM:
			PO_G12_L;
		  PO_MCMM_L;
			break;
		
		default:
			break;
		
	}
	
	HC595_Update(slot);
	
	mdc_bus_select(SLOT_NUM);
}

void mdc_ana_task(mdc_slot_t slot)
{
	
}











