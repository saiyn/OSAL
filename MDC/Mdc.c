#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "Mdc.h"
#include "System_Task.h"
#include "OSAL_RingBuf.h"
#include "NAD_Uart_App_Task.h"
#include "OSAL_Timer.h"
#include "Soft_Timer_Task.h"
#include "OSAL_Soft_IIC.h"
#include "EPRom_driver.h"
#include "Src4382.h"

mdc_t *gMdc;
//mdc_setup_t mdc_setup[SLOT_NUM];

extern mdc_slot_t gSlot;

#define MDC_TEST_TRACE   nad_send_str
#define MDC_WAIT_FOR_CONFIRM_TIMEOUT  1000
#define MDC_WAIT_FOR_CONFIRM_CNT      5
#define MDC_WAIT_FOR_INSERT_CABLE_CNT      10
#define MDC_WAIT_FOR_USB_CONFIRM_CNT      10
#define MDC_WAIT_FOR_VOL_CONFIRM_CNT      10


static result_t do_mdc_test(mdc_test_t *p, bool user_msg, uint8 msg);


const static char *mdc_test_item[TEST_ITEM_NUM]={
	"I2C test",
	"GPIO test",
	"SPI test",
	"Setup OPT Input and test",
	"Setup COAX Input and test",
	"Setup usb and test",
	"Voltag test",
	"Current test",
	"Card type test",
	"Report "
};

const static char *mdc_test_result[MDC_TEST_NUM]={
	"Not Judged",
	"Fail",
	"Pass",
	"Wait"
};

typedef enum{
	CON_SKIP=0,
	CON_START,
	CON_RESTART,
	CON_NULL
}confirm_result_t;
	

ring_buffer_t mdc_slot1;
ring_buffer_t mdc_slot2;

char *CardTypeName[MDC_CARD_NUM + 1] =
{
    "Digital",
    "USB2.0",
    "HDMI",
    "Analog",
    "BluOS",
    "HDMI2.0",
    "Test",
};

const static uint16 Resistance_Table[MDC_CARD_NUM]={
	  6800,                     // dio
    510,                      // usb_xmos  //usb2.0
    8200,                     // hdmi
    3300,                     // ana
    16000,                    // blueOS
    24000,                    // hdmi_v2
};

/*10k*/
#define RES_VAL            10000ul
/*1% precision*/
#define RES_PRECISION    5

#define ADC_PRECISION    12

#define RES2ADC(x)        ((x*(2 << (ADC_PRECISION - 1)))/(x+RES_VAL))

static bool mdc_adc_cmp(uint16 adc, uint16 res)
{
	uint32 v_adc, adc_l, adc_h;
  uint32 d_res, d_adc;
	
	v_adc = RES2ADC(res);
	d_res = res * RES_PRECISION / 100;
	d_adc = RES2ADC(d_res);
  adc_h = v_adc + d_adc;
	adc_l = v_adc > d_adc ? (v_adc - d_adc) : 0;
	
	//SYS_TRACE("v_adc= %d, adc_h=%d, adc=%d, adc_l=%d\r\n",v_adc, adc_h, adc, adc_l);
	
	if(adc >= adc_l && adc < adc_h){
		return true;
	}else{
		return false;
	}
	
}


static int mdc_slot_init(mdc_slot_state_t *slot, mdc_type_t type, uint8 index)
{
	OSAL_ASSERT(index < SLOT_NUM);
	
	slot->index = index;
	
	switch(type){
		case MDC_DIO:
				slot->type = MDC_DIO;
        slot->max_input = DIO_INPUT_N;
        slot->str_cmd = DEF_DIO_CMD[index];
        slot->input = DIO_INPUT_COAX1;
			break;
		
		case MDC_USB:
				slot->type = MDC_USB;
        slot->max_input = USB_INPUT_N;
        slot->str_cmd = DEF_USB_CMD[index];
        slot->input = USB_INPUT_PC;
			break;
		
		case MDC_HDMI:
				slot->type = MDC_HDMI;
        slot->max_input = HDMI_INPUT_N;
        slot->str_cmd = DEF_HDMI_CMD[index];
        slot->input = HDMI_INPUT_1;
			break;
		
		case MDC_ANA:
				slot->type = MDC_ANA;
        slot->max_input = ANA_INPUT_N;
        slot->str_cmd = DEF_ANA_CMD[index];
        slot->input = ANA_INPUT_PHONO;
			break;
		
		case MDC_BLUOS:
				slot->type = MDC_BLUOS;
        slot->max_input = BT_INPUT_N;
        slot->str_cmd = DEF_BLU_CMD[index];
        slot->input = BT_INPUT;
			break;
		
		case MDC_HDMI_v2:
				slot->type = MDC_HDMI_v2;
        slot->max_input = HDMI_V2_INPUT_N;
        slot->str_cmd = DEF_HDMI_V2_CMD[index];
        slot->input = HDMI_V2_INPUT_1;
			break;
		
		default:
			break;
	}
	
	return 0;
}

static void mdc_slot_update(mdc_slot_state_t *pslot, mdc_slot_state_t *cslot)
{
	if(cslot){
		pslot->index = cslot->index;
		pslot->type = cslot->type;
		pslot->name = cslot->name;
		pslot->max_input = cslot->max_input;
	}else{
		pslot->type = MDC_CARD_NUM;
	}
}

static int mdc_setup_init (mdc_slot_state_t *slot, mdc_setup_t* setup, mdc_type_t type, uint8 index, uint8 offset, uint8 slot_num)
{
	uint8 i;
	uint8 len;
	
	OSAL_ASSERT(index < SLOT_NUM);
	
	setup->index = index;
	
	switch(type){
		case MDC_DIO:
			setup->type = type;
			for(i=0; i<DIO_INPUT_N; i++){
				strcpy(setup->card.dio.name[i], DEF_DIO_NAME[1][i]);
				len = strlen(DEF_DIO_NAME[1][i]);
				OSAL_ASSERT(len + 1 < MAX_INPUT_NAME_LEN);
				//if(offset){
					setup->card.dio.name[i][len] = 'A' + slot_num;
//				}else{
//					setup->card.dio.name[i][len] = 'A';
//				}
				setup->card.dio.name[i][len + 1] = 0;
				setup->card.dio.enable[i] = true;
				setup->card.dio.volume_control[i] = 0; //TBD
				setup->card.dio.volume_fixed[i] = 0; //TBD
			}
			
		  slot->name = &(setup->card.dio.name);
			
			break;
			
		case MDC_USB:
			setup->type = type;
		  for(i=0; i<USB_INPUT_N; i++){
				strcpy(setup->card.usb.name[i], DEF_USB_NAME[offset][i]);
				
				if(offset && i != USB_INPUT_FRONT){
					 len = strlen(DEF_USB_NAME[offset][i]);
					 OSAL_ASSERT(len + 1 < MAX_INPUT_NAME_LEN);
					 setup->card.usb.name[i][len] = 'A' + slot_num;
					 setup->card.usb.name[i][len + 1] = 0;
				}
				
				if (offset){
           if ((index > 0)&&(i == USB_INPUT_FRONT))
            {
               setup->card.usb.enable[i] = false;
            }
            else
            {
               setup->card.usb.enable[i] = true;
            }
        }else{
            setup->card.usb.enable[i] = true;
        }
        setup->card.usb.volume_control[i] = 0;//TBD
        setup->card.usb.volume_fixed[i] = 0;//TBD
			}
			
			slot->name = &(setup->card.usb.name);
			break;
			
		case MDC_HDMI:
			setup->type = type;
		  for(i=0; i<HDMI_INPUT_N; i++){
				strcpy(setup->card.hdmi.name[i], DEF_HDMI_NAME[offset][i]);
				if(offset){
					len = strlen(DEF_HDMI_NAME[offset][i]);
					OSAL_ASSERT(len + 1 < MAX_INPUT_NAME_LEN);
					setup->card.hdmi.name[i][len] = 'A'+ slot_num;
					setup->card.hdmi.name[i][len + 1] = 0;
				}
				setup->card.hdmi.enable[i] = true;
				setup->card.hdmi.volume_control[i] = 0; //TBD
				setup->card.hdmi.volume_fixed[i] = 0;  //TBD
			}
			
			slot->name = &(setup->card.hdmi.name);
			break;
			
		case MDC_ANA:
			setup->type = type;
		  for(i=0; i<ANA_INPUT_N; i++){
				strcpy(setup->card.ana.name[i], DEF_ANA_NAME[offset][i]);
				if(offset){
					len = strlen(DEF_ANA_NAME[offset][i]);
					OSAL_ASSERT(len + 1 < MAX_INPUT_NAME_LEN);
					setup->card.ana.name[i][len] = 'A' + slot_num;
					setup->card.ana.name[i][len + 1] = 0;
				}
				
				setup->card.ana.enable[i] = true;
				setup->card.ana.volume_control[i] = 0; //TBD
				setup->card.ana.volume_fixed[i] = 0; //TBD
				setup->card.ana.gain[i] = 0; //TBD
				setup->card.ana.adc_sr[i] = 0; //TBD
			}
			setup->card.ana.gain[ANA_INPUT_PHONO] = 0;
      setup->card.ana.phono_mode = PHONO_MM;
			
			slot->name = &(setup->card.ana.name);
			break;
			
		case MDC_BLUOS:
			setup->type = type;
			for(i=0; i<BT_INPUT_N; i++){
				strcpy(setup->card.blu.name[i], DEF_BLU_NAME[offset][i]);
				if(offset){
					len = strlen(DEF_BLU_NAME[offset][i]);
					OSAL_ASSERT(len + 1 < MAX_INPUT_NAME_LEN);
					setup->card.blu.name[i][len] = 'A' + slot_num;
					setup->card.blu.name[i][len + 1] = 0;
				}
				setup->card.blu.enable[i] = true;
				setup->card.blu.volume_control[i] = 0;//TBD
				setup->card.blu.volume_fixed[i] = 0; //TBD
			}
			
			slot->name = &(setup->card.blu.name);
			break;
			
		case MDC_HDMI_v2:
			setup->type = type;
		  for(i=0; i<HDMI_V2_INPUT_N; i++){
				strcpy(setup->card.hdmi2.name[i], DEF_HDMI_V2_NAME[offset][i]);
				if(offset){
					len = strlen(DEF_HDMI_V2_NAME[offset][i]);
					OSAL_ASSERT(len + 1< MAX_INPUT_NAME_LEN);
					setup->card.hdmi2.name[i][len - 1] = 'A' + slot_num;
					setup->card.hdmi2.name[i][len + 1] = 0;
				}
				setup->card.hdmi2.enable[i] = true;
				setup->card.hdmi2.volume_control[i] = 0;
				setup->card.hdmi2.volume_fixed[i] = 0;
			}
			
		  slot->name = &(setup->card.hdmi2.name);
			break;
			
		 default:
				memset(setup, 0, sizeof(mdc_setup_t));
		    setup->type = MDC_CARD_NUM;
       break;		 
		 
	}
	
	return 0;
}

static void mdc_card_name_update(mdc_slot_state_t *slot, mdc_setup_t* setup)
{
  switch(slot->type){
		case MDC_DIO:
		  slot->name = &(setup->card.dio.name);
			break;
			
		case MDC_USB:		
			slot->name = &(setup->card.usb.name);
			break;
			
		case MDC_HDMI:
			slot->name = &(setup->card.hdmi.name);
			break;
			
		case MDC_ANA:
			slot->name = &(setup->card.ana.name);
			break;
			
		case MDC_BLUOS:			
			slot->name = &(setup->card.blu.name);
			break;
			
		case MDC_HDMI_v2:
		  slot->name = &(setup->card.hdmi2.name);
			break;
			
		 default:
       break;		 
	}
	
}

static int mdc_adc_load(uint16 *buf, uint8 channle)
{
	OSAL_ASSERT(buf != NULL); 
	OSAL_ASSERT(channle <= SLOT_NUM);
	
	bsp_adc0_load(buf, channle, 0);
	
	return 0;
}

static void mdc_prepare_test_environment(sys_state_t *sys, uint8 slot)
{
	mdc_test_t *p = NULL;
	
	/*only do this once*/
	if(sys->mdc_test == NULL){
		p = (mdc_test_t *)MALLOC(sizeof(mdc_test_t));
		OSAL_ASSERT(p != NULL);
		memset(p, 0, sizeof(mdc_test_t));
		p->start_index = slot;
		p->cur_index = slot;
		p->item = TEST_ITEM_I2C;
		sys->mdc_test = (void *)p;
//		AC_STANDBY(1);
//	  DC5V_EN(1);
//	  DC3V_EN(1);
		mdc_power_set((mdc_slot_t)slot, ON);
		SYS_TRACE("mdc test environment prepared.\r\n");
	}
	
	p->bitmap_insert |= (1 << slot);
	p->num++;
}

int mdc_init(void)
{
	/*the index of the slot*/
	int i;
	/*the index of the card*/
  int j;
  int type_index[MDC_CARD_NUM] = {0};
  int type_count[MDC_CARD_NUM] = {0};
	
	mdc_adc_load(gMdc->adc, SLOT_NUM);
  //mdc_adc_load(gMdc->adc, SLOT_NUM);

	for (i = 0; i < SLOT_NUM; i++){ 
    for (j = 0; j < MDC_CARD_NUM; j++){
      if (mdc_adc_cmp(gMdc->adc[i], Resistance_Table[j]) == true){
        if ((j == MDC_HDMI)||(j == MDC_HDMI_v2)){
            type_count[MDC_HDMI]++;
            type_count[MDC_HDMI_v2]++;
         }else{
            type_count[j]++;
         }
				 
				 /*used for firmware upgrade*/
				 gSlot = i;
      }
    }
  }
		
	for (i = 0; i < SLOT_NUM; i++){
    for (j = 0; j < MDC_CARD_NUM; j++){
      if(mdc_adc_cmp(gMdc->adc[i], Resistance_Table[j]) == true){
        mdc_slot_init(&gMdc->slot[i], (mdc_type_t)j, type_index[j]);
        gMdc->slot_src_idx[i] = gSystem_t->src_num;
        gSystem_t->src_num += (gMdc->slot[i].max_input);
        if ((gMdc->prev_slot[i].type != gMdc->slot[i].type) || (gMdc->prev_slot[i].index != gMdc->slot[i].index)){
          /*--- mdc changed, setup default ---*/
          SYS_TRACE("slot[%d], type %s changed, setup default\r\n", i,CardTypeName[gMdc->slot[i].type]);
          if (type_count[j] > 1){
						/*there have 2 the same mdc cards. so the first one's name should init with indicator number.*/
            mdc_setup_init(&gMdc->slot[i], &gMdc->setup[i], (mdc_type_t)j, gMdc->slot[i].index, 1, i);
          }
          else{
            mdc_setup_init(&gMdc->slot[i], &gMdc->setup[i], (mdc_type_t)j, gMdc->slot[i].index, 0, i);
          }
          if(gMdc->mdc_slot_index == i){
            gMdc->mdc_slot_src_index = 0;
          }
					
					mdc_slot_update(&gMdc->prev_slot[i], &gMdc->slot[i]);
					Send_SysTask_Msg(SYS_DATABASE_SAVE_MSG, 1);
        }else{
					/*it is very important to add this handle*/
					mdc_card_name_update(&gMdc->prev_slot[i], &gMdc->setup[i]);
					//mdc_slot_update(&gMdc->prev_slot[i], &gMdc->slot[i]);
				}
                
         if ((j == MDC_HDMI)||(j == MDC_HDMI_v2))
         {
            type_index[MDC_HDMI]++;
            type_index[MDC_HDMI_v2]++;
         }
         else
         {
            type_index[j]++;
         }
                
				sys_src_list_append(gSrcListHead, &gMdc->setup[i], &gMdc->prev_slot[i]);
				 
        break;
      }
    }
			/*no normal MDC card found*/
			if(gMdc->slot[i].type == MDC_CARD_NUM){
					mdc_setup_init(&gMdc->slot[i] ,&gMdc->setup[i], MDC_CARD_NUM, 0, 0, i);
					if (gMdc->adc[i] < 20){
						SYS_TRACE("slot[%d] found[%s] card\r\n", i, CardTypeName[MDC_CARD_NUM]);
						mdc_prepare_test_environment(gSystem_t, i);
					}else{
						SYS_TRACE("No MDC card found\r\n");
					}
					
				 mdc_slot_update(&gMdc->prev_slot[i], NULL);
				 Send_SysTask_Msg(SYS_DATABASE_SAVE_MSG, 1);
			}
			else
			{
				 SYS_TRACE("slot[%d] found[%s] card\r\n", i, CardTypeName[gMdc->slot[i].type]);
			}
    }
	
	return 0;
}


void mdc_bus_select(mdc_slot_t slot)
{
	
//	SYS_TRACE("mdc_bus_select:%d\r\n", slot);
	
	 MDC_PS01_H;
	 MDC_PS02_H;
	 MDC_PM0EN_L;
	 MDC_PM1EN_L;
	
	 switch(slot){
		 case SLOT_1:
			 MDC_PM0EN_H;
		   MDC_PS01_L;
			 break;
		 
		 case SLOT_2:
			 MDC_PM1EN_H;
		   MDC_PS02_L;
			 break;
		 
		 default:
			 break;
	 }
}

void mdc_psX_set(mdc_slot_t slot, uint8 val)
{
	switch(slot){
		 case SLOT_1:
			 if(val) MDC_PS01_H;
		   else MDC_PS01_L;
		   
			 break;
		 
		 case SLOT_2:
			 if(val) MDC_PS02_H;
		   else MDC_PS02_L;
			 break;
		 
		 default:
			 break;
	 }
	
}

/*0->output 1->input*/
void mdc_gpio_set_dir(mdc_slot_t slot, uint8 dir)
{
	switch(slot){
		 case SLOT_1:
			 if(dir == 0){
				 GPIOPinTypeGPIOOutput(GPIO1M0_PORT, GPIO1M0_PIN);
				 GPIOPinTypeGPIOOutput(GPIO2M0_PORT, GPIO2M0_PIN);
				 GPIOPinTypeGPIOOutput(GPIO3M0_PORT, GPIO3M0_PIN);
				 GPIOPinTypeGPIOOutput(GPIO4M0_PORT, GPIO4M0_PIN);
			 }else{
				 GPIOPinTypeGPIOInput(GPIO1M0_PORT, GPIO1M0_PIN);
				 GPIOPinTypeGPIOInput(GPIO2M0_PORT, GPIO2M0_PIN);
				 GPIOPinTypeGPIOInput(GPIO3M0_PORT, GPIO3M0_PIN);
				 GPIOPinTypeGPIOInput(GPIO4M0_PORT, GPIO4M0_PIN);
			 }
			 break;
		 
		 case SLOT_2:
			 if(dir == 0){
				 GPIOPinTypeGPIOOutput(GPIO1M1_PORT, GPIO1M1_PIN);
				 GPIOPinTypeGPIOOutput(GPIO2M1_PORT, GPIO2M1_PIN);
				 GPIOPinTypeGPIOOutput(GPIO3M1_PORT, GPIO3M1_PIN);
				 GPIOPinTypeGPIOOutput(GPIO4M1_PORT, GPIO4M1_PIN);
			 }else{
				 GPIOPinTypeGPIOInput(GPIO1M1_PORT, GPIO1M1_PIN);
				 GPIOPinTypeGPIOInput(GPIO2M1_PORT, GPIO2M1_PIN);
				 GPIOPinTypeGPIOInput(GPIO3M1_PORT, GPIO3M1_PIN);
				 GPIOPinTypeGPIOInput(GPIO4M1_PORT, GPIO4M1_PIN);
			 }
			 break;
		 
		 default:
			 break;
	 }
}

uint8 mdc_gpio_get(mdc_slot_t slot, mdc_gpio_t gpio)
{
	
	switch(slot){
		 case SLOT_1:
			 if(MDC_GPIO_1 == gpio) return GPIO_ReadSinglePin(GPIO1M0_PORT, GPIO1M0_PIN);
			 if(MDC_GPIO_2 == gpio) return GPIO_ReadSinglePin(GPIO2M0_PORT, GPIO2M0_PIN);
			 if(MDC_GPIO_3 == gpio) return GPIO_ReadSinglePin(GPIO3M0_PORT, GPIO3M0_PIN);
			 if(MDC_GPIO_4 == gpio) return GPIO_ReadSinglePin(GPIO4M0_PORT, GPIO4M0_PIN);
		   
			 break;
		 
		 case SLOT_2:
			 if(MDC_GPIO_1 == gpio) return GPIO_ReadSinglePin(GPIO1M1_PORT, GPIO1M1_PIN);
			 if(MDC_GPIO_2 == gpio) return GPIO_ReadSinglePin(GPIO2M1_PORT, GPIO2M1_PIN);
			 if(MDC_GPIO_3 == gpio) return GPIO_ReadSinglePin(GPIO3M1_PORT, GPIO3M1_PIN);
			 if(MDC_GPIO_4 == gpio) return GPIO_ReadSinglePin(GPIO4M1_PORT, GPIO4M1_PIN);
			 break;
		 
		 default:
			 break;
	 }
	
	return  -1;
}

void mdc_iis_select(mdc_slot_t slot)
{
	  switch(slot){
		 case SLOT_1:
			 MDC_IIS_L;
			 break;
		 
		 case SLOT_2:
			 MDC_IIS_H;
			 break;
		 
		 default:
			 break;
	 }
}

void mdc_usb_bus_select(mdc_slot_t slot)
{
	MDC_M0M1USBEN(1);
	
	switch(slot){
		 
		 case SLOT_1:
       MDC_M0M1USBEN(0);
		   MDC_M0M1USBS(0);
			 break;
		 
		 case SLOT_2:
       MDC_M0M1USBEN(0);
		   MDC_M0M1USBS(1);
			 break;
			 
			default:
				break;
			
		}

}


void mdc_usb_disable(mdc_slot_t slot)
{
	switch(slot){
		 
		 case SLOT_1:
       MDC_M0M1USBEN(1);
			 break;
		 
		 case SLOT_2:
       MDC_M0M1USBEN(1);
			 break;
			 
			default:
				break;
			
		}
}

void mdc_power_set(mdc_slot_t slot, uint8 state)
{
	 switch(slot){
		 
		 case SLOT_1:
       if(ON == state){
				 MDC_M0PS_ON_H;
			 }else{
				 MDC_M0PS_ON_L;
			 }
			 break;
		 
		 case SLOT_2:
			 if(ON == state){
				 MDC_M1PS_ON_H;
			 }else{
				 MDC_M1PS_ON_L;
			 }
			 break;
		 
		 default:
			 if(ON == state){
				 MDC_M0PS_ON_H;
				 MDC_M1PS_ON_H;
			 }else{
				 MDC_M0PS_ON_L;
				 MDC_M1PS_ON_L;
			 }
			 break;
	 }
}


void mdc_pa_set(uint8 val)
{
	 if(val & 0x01){
		 MDC_PA0_H;
	 }else{
		 MDC_PA0_L;
	 }
	 
	 if(val & 0x02){
		 MDC_PA1_H;
	 }else{
		 MDC_PA1_L;
	 }
	 
	 if(val & 0x04){
		 MDC_PA2_H;
	 }else{
		 MDC_PA2_L;
	 }
}
	

uint8 mdc_int_get(mdc_slot_t slot)
{
	uint8 ret = 0;
	
	switch(slot){
		case SLOT_1:
			ret = GPIO_PIN_GET(MDC_INTM0_PORT, MDC_INTM0_PIN);
			break;
		
		case SLOT_2:
			ret = GPIO_PIN_GET(MDC_INTM1_PORT, MDC_INTM1_PIN);
			break;
		
		default:
			break;
	}
	
	return ret;
}

static bool do_test_report(mdc_test_t *p)
{
	uint8 j;
	
	MDC_TEST_TRACE("\r\n***************MDC Test Report[slot:%d]**********************\r\n", p->cur_index);
	for(j = 0; j < TEST_ITEM_REPORT_RESULT; j++){
		MDC_TEST_TRACE("%-*s -> %s\r\n", 25, mdc_test_item[j], mdc_test_result[p->report[j]]);
	}
	
	MDC_TEST_TRACE("\r\n**************************************************************\r\n");
	
	/*check next slot*/
	for(j = p->start_index + 1; j < SLOT_NUM; j++){
		if(p->bitmap_insert & (1 << j)){
			p->cur_index = j;
			p->item = TEST_ITEM_I2C;
			mdc_power_set((mdc_slot_t)j, ON);
			osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
			return true;
		}
	}
	
	return false;
}

static bool test_start_handler(sys_state_t *sys)
{
	mdc_test_t *p = (mdc_test_t *)(sys->mdc_test);
	
	if(p->item == TEST_ITEM_REPORT_RESULT){
		return do_test_report(p);
	}else{
	
		MDC_TEST_TRACE("\r\n\r\n*******************************************************************\r\n");
		MDC_TEST_TRACE("In MDC Test Mode\r\n");
		MDC_TEST_TRACE("Total Cards:[%d] Start Slot:[%d] Current Slot:[%d]\r\n", p->num, p->start_index, p->cur_index);
		MDC_TEST_TRACE("[%s]\r\n", mdc_test_item[p->item]);
		MDC_TEST_TRACE("\r\n\r\n");

		p->report[p->item] = MDC_TEST_NOT_JUDGED;
		
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, MDC_WAIT_FOR_CONFIRM_TIMEOUT);
	}
	
	return true;
	
}


static confirm_result_t test_wait_for_start_confirm_handler(sys_state_t *sys, bool user_msg, uint8 msg)
{
	static uint8 cnt = MDC_WAIT_FOR_CONFIRM_CNT;
	mdc_test_t *p = (mdc_test_t *)(sys->mdc_test);
	
	if(user_msg && msg == 'x'){
		MDC_TEST_TRACE("\r\nSkip %s\r\n", mdc_test_item[p->item]);
		p->item++;
		cnt = MDC_WAIT_FOR_CONFIRM_CNT;
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
		return CON_SKIP;
	}else{
	
		MDC_TEST_TRACE("\033[2K\r");
		MDC_TEST_TRACE("Press 'x' to skip, or will start automatically after %ds", cnt--);
		
		if(cnt == 0){
			cnt = MDC_WAIT_FOR_CONFIRM_CNT;
			osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
			return CON_START;
		}else{
			osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, MDC_WAIT_FOR_CONFIRM_TIMEOUT);
		}
	}
	
	return CON_NULL;
}

static confirm_result_t test_wait_for_restart_confirm_handler(sys_state_t *sys, bool user_msg, uint8 msg)
{
	static uint8 cnt = MDC_WAIT_FOR_CONFIRM_CNT;
	mdc_test_t *p = (mdc_test_t *)(sys->mdc_test);
	
	if(user_msg && msg == 'r'){
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
		cnt = MDC_WAIT_FOR_CONFIRM_CNT;
		return CON_RESTART;
	}else{
	
		MDC_TEST_TRACE("\033[2K\r");
		MDC_TEST_TRACE("Press 'r' to restart, or will start next automatically after %ds", cnt--);
		
		if(cnt == 0){
			cnt = MDC_WAIT_FOR_CONFIRM_CNT;
			p->item++;
			osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
			return CON_SKIP;
		}else{
			osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, MDC_WAIT_FOR_CONFIRM_TIMEOUT);
		}
	}
	
	return CON_NULL;
}


static result_t test_process_handler(sys_state_t *sys, bool user_msg, uint8 msg)
{
	result_t result;
	mdc_test_t *p = (mdc_test_t *)(sys->mdc_test);
	
	result = do_mdc_test(p, user_msg, msg);
	
	p->report[p->item] = result;
	
	if(MDC_TEST_FAIL == result){
		MDC_TEST_TRACE("\r\n\r\n%s ->Fail!!!\r\n\r\n", mdc_test_item[p->item]);
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
	}else if(MDC_TEST_PASS == result){
		MDC_TEST_TRACE("\r\n\r\n%s ->Pass\r\n\r\n", mdc_test_item[p->item]);
		p->item++;
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
	}else if(MDC_TEST_WAIT == result){
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 1000);
	}else{
		MDC_TEST_TRACE("\r\n\r\n%s ->Unjudjed\r\n\r\n", mdc_test_item[p->item]);
		osal_start_timerEx(gSTTID, MDC_TEST_POLL_MSG, 100);
	}

	
	
	return result;
}

void mdc_test_task_handler(sys_state_t *sys, bool user_msg, uint8 msg)
{
	static mdc_test_task_state_t state = TEST_START;
	confirm_result_t retval;
	result_t result;
	mdc_test_t *p = (mdc_test_t *)(sys->mdc_test);

	switch(state){
		
		case TEST_START:
			if(test_start_handler(sys) == true){
				state = TEST_WAIT_FOR_START_CONFIRM;
			}
			break;
		
		case TEST_WAIT_FOR_START_CONFIRM:
			retval = test_wait_for_start_confirm_handler(sys, user_msg, msg);
		  if(CON_START == retval){
				state = TEST_IN_PROCESS;
			}else if(CON_SKIP == retval){
				state = TEST_START;
			}
			break;
			
		case TEST_IN_PROCESS:		
			result = test_process_handler(sys, user_msg, msg);
			if(result == MDC_TEST_PASS){
				state = TEST_START;
			}else if(result == MDC_TEST_FAIL || result == MDC_TEST_NOT_JUDGED){
				state = TEST_WAIT_FOR_RESTART_CONFIRM;	
			}
			break;
			
		case TEST_WAIT_FOR_RESTART_CONFIRM:
			retval = test_wait_for_restart_confirm_handler(sys, user_msg, msg);
		  if(CON_RESTART == retval || CON_SKIP == retval){
				state = TEST_START;
			}
			break;
		
		default:
			break;
		
	}

}

static result_t do_i2c_test(uint8 slot)
{
#define MDC_IIC_TEST_STR  	"MDC IIC TEST"
	
  uint8 buf[16] = {0};
	int retval;
  iic_device_t device;

  if(SLOT_1 == slot){
		device = MDC_SLOT1_IIC;
	}else if(SLOT_2 == slot){
		device = MDC_SLOT2_IIC;
	}else{
		return MDC_TEST_FAIL;
	}
  
	retval = eeprom_write(device, 0, (uint8 *)MDC_IIC_TEST_STR, strlen(MDC_IIC_TEST_STR));
	
	if(retval == strlen(MDC_IIC_TEST_STR)){
		retval = eeprom_read(device, 0, buf, strlen(MDC_IIC_TEST_STR));
		if(retval == strlen(MDC_IIC_TEST_STR)){
			if(!strcmp((char *)buf, MDC_IIC_TEST_STR)){
				return MDC_TEST_PASS;
			}
				
		}
	}
	
	return MDC_TEST_FAIL;
}

static result_t do_gpio_test(uint8 slot)
{
	uint8 record = 0;
	uint8 j;
	result_t result = MDC_TEST_PASS;
	
	mdc_bus_select(slot);
	
	/*1->input*/
	mdc_gpio_set_dir(slot, 1);
	
	mdc_pa_set(0x07);
	mdc_psX_set(slot, 1);
	
	for(j = 0; j < MDC_GPIO_NUM; j++){
		if(mdc_gpio_get(slot, (mdc_gpio_t)j) == 0){
			record |= (1 << j);
		}
	}
	
	if(mdc_int_get(slot) == 0){
		record |= (1 << MDC_GPIO_NUM);
	}
	
	mdc_pa_set(0x0);
	mdc_psX_set(slot, 0);
	
	for(j = 0; j < MDC_GPIO_NUM; j++){
		if(mdc_gpio_get(slot, (mdc_gpio_t)j) == 1){
			record |= (1 << j);
		}
	}
	
	if(mdc_int_get(slot) == 1){
		record |= (1 << MDC_GPIO_NUM);
	}
	
	mdc_bus_select(SLOT_NUM);
	
	for(j = 0; j < MDC_GPIO_NUM; j++){
		
		if(record & (1 << j)){
			if(j == 0){
				MDC_TEST_TRACE("GPIO1 or PS test ->Fail!!!\r\n");
			}else{
				MDC_TEST_TRACE("GPIO%d or PA%d test ->Fail!!!\r\n", (j + 1), (3 - j));
			}
			
			result = MDC_TEST_FAIL;
		}
		
	}
	
	if(record & (1 << MDC_GPIO_NUM)){
		MDC_TEST_TRACE("INT or PA0 test ->Fail!!!\r\n");
		
		result = MDC_TEST_FAIL;
	}
	
	
	return result;
}

static void test_input_select(test_input_t input)
{
	static const UINT8 in_table[TEST_INPUT_N]={3, 4, 4, 4};
	uint8 in;
	
	in = in_table[(unsigned char)input];
	AK4118_write(0x02, 0x80|(in<<4));
	AK4118_write(0x03, 0x40|in);

}

static int ak4118_on_test_card_init(uint8 slot, uint8 ch)
{
	spi_ctl_t config = {8, 0, 400000};
  int retval = 0;
	
	mdc_bus_select((mdc_slot_t)slot);
	
	bsp_spi0_ctl(&config);

  retval += AK4118_write(0x00, 0x42);
  bsp_delay_ms(2);
	retval += AK4118_write(0x00, 0x43);
	bsp_delay_ms(2);
  retval += AK4118_write(0x01, 0x5A);        /* audio format i2s */
	retval += AK4118_write(0x02, 0x80);
	retval += AK4118_write(0x21, 0x02);        /* gp1 output*/
	retval += AK4118_write(0x22, 0x00);

  test_input_select((test_input_t)ch);

  mdc_bus_select(SLOT_NUM);

	config.protocal = 3;
  bsp_spi0_ctl(&config);

	return retval;
}

static result_t do_spi_test(uint8 slot)
{
	result_t result = MDC_TEST_PASS;
	int retval = 0;
	
	retval = ak4118_on_test_card_init(slot, 0);
	
	if(retval) result = MDC_TEST_FAIL;
	
	return result;
}

static result_t do_opt_test(uint8 slot, bool user_msg)
{
	static enum{
		OPT_START=0,
		OPT_WAIT_FOR_OPT_INSERT,
	}opt_state = OPT_START;
	
	result_t result = MDC_TEST_NOT_JUDGED;
	int retval = 0;
	static uint8 cnt = MDC_WAIT_FOR_INSERT_CABLE_CNT;
	
	switch(opt_state){
		
		case OPT_START:
			retval = ak4118_on_test_card_init(slot, TEST_INPUT_OPT);
	
	    if(retval){
				return result;
			}
			
			if(src4382_audio_detect(0,0)){
				result = MDC_TEST_PASS;
			}else{
				opt_state = OPT_WAIT_FOR_OPT_INSERT;
				result = MDC_TEST_WAIT;
			}
			break;
		
		case OPT_WAIT_FOR_OPT_INSERT:
			if(true == user_msg){
				opt_state = OPT_START;
				result = MDC_TEST_PASS;
				cnt = MDC_WAIT_FOR_INSERT_CABLE_CNT;
			}else{
				if(cnt--){
					MDC_TEST_TRACE("\033[2K\r");
					MDC_TEST_TRACE("Please insert optical cable, or %ds later this test will be judged to be Fail", cnt);
					result = MDC_TEST_WAIT;
				}else{
					result = MDC_TEST_FAIL;
					cnt = MDC_WAIT_FOR_INSERT_CABLE_CNT;
				}
			}
		
			break;
		
	}
	
	return result;
}


static result_t do_usb_test(uint8 slot, bool user_msg, uint8 msg)
{
	static enum{
		USB_START=0,
		USB_WAIT_FOR_CONFIRM,
	}opt_state = USB_START;
	
	result_t result = MDC_TEST_NOT_JUDGED;
	int retval = 0;
	static uint8 cnt = MDC_WAIT_FOR_VOL_CONFIRM_CNT;
	
	switch(opt_state){
		
		case USB_START:
	    
				opt_state = USB_WAIT_FOR_CONFIRM;
				result = MDC_TEST_WAIT;
			
			break;
		
		case USB_WAIT_FOR_CONFIRM:
			if(true == user_msg && msg == 'y'){
				opt_state = USB_START;
				result = MDC_TEST_PASS;
				cnt = MDC_WAIT_FOR_VOL_CONFIRM_CNT;
			}else if(true == user_msg && msg == 'n'){
				opt_state = USB_START;
				result = MDC_TEST_FAIL;
				cnt = MDC_WAIT_FOR_VOL_CONFIRM_CNT;
			}else{
				if(cnt--){
					MDC_TEST_TRACE("\033[2K\r");
					MDC_TEST_TRACE("Does the computer find the usb device and if found press 'y' or press 'n'[%ds]", cnt);
					
					result = MDC_TEST_WAIT;
				}else{
					result = MDC_TEST_FAIL;
					cnt = MDC_WAIT_FOR_VOL_CONFIRM_CNT;
				}
			}
		
			break;
		
	}

	return result;
}

static result_t do_voltage_test(uint8 slot, bool user_msg, uint8 msg)
{
	static enum{
		VOL_START=0,
		VOL_WAIT_FOR_CONFIRM,
	}opt_state = VOL_START;
	
	result_t result = MDC_TEST_NOT_JUDGED;
	int retval = 0;
	static uint8 cnt = MDC_WAIT_FOR_USB_CONFIRM_CNT;
	
	switch(opt_state){
		
		case VOL_START:
	    
				opt_state = VOL_WAIT_FOR_CONFIRM;
				result = MDC_TEST_WAIT;
			
			break;
		
		case VOL_WAIT_FOR_CONFIRM:
			if(true == user_msg && msg == 'y'){
				opt_state = VOL_START;
				result = MDC_TEST_PASS;
				cnt = MDC_WAIT_FOR_USB_CONFIRM_CNT;
			}else if(true == user_msg && msg == 'n'){
				opt_state = VOL_START;
				result = MDC_TEST_FAIL;
				cnt = MDC_WAIT_FOR_USB_CONFIRM_CNT;
			}else{
				if(cnt--){
					MDC_TEST_TRACE("\033[2K\r");
					MDC_TEST_TRACE("Please add 3A or 2A load, and measure the voltag.(3A: >12V & 2A: >12.3V) if OK press 'y' or press 'n'[%ds]", cnt);
					result = MDC_TEST_WAIT;
				}else{
					result = MDC_TEST_FAIL;
					cnt = MDC_WAIT_FOR_USB_CONFIRM_CNT;
				}
			}
		
			break;
		
	}

	return result;
}

static result_t do_mdc_test(mdc_test_t *p, bool user_msg, uint8 msg)
{
	result_t result = MDC_TEST_NOT_JUDGED;
	
	switch(p->item){
		
		case TEST_ITEM_I2C:
			result = do_i2c_test(p->cur_index);
			break;
		
		case TEST_ITEM_GPIO:
			result = do_gpio_test(p->cur_index);
			break;
		
		case TEST_ITEM_SPI:
			result = do_spi_test(p->cur_index);
			break;
		
		case TEST_ITEM_OPT:
			result = do_opt_test(p->cur_index, user_msg);
			break;
		
		case TEST_ITEM_USB:
			result = do_usb_test(p->cur_index, user_msg, msg);
			break;
		
		case TEST_ITEM_VOLTAGE:
			result = do_voltage_test(p->cur_index, user_msg, msg);
			break;
		
		
		
		default:
			break;
		
	}
	
	return result;
}


void mdc_slot1_sda(int level)
{
	 if(level){
		GPIOPinTypeGPIOInput(MDC_SDA0_PORT, MDC_SDA0_PIN);
		GPIO_PIN_SET(MDC_SDA0_PORT, MDC_SDA0_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(MDC_SDA0_PORT, MDC_SDA0_PIN);
		GPIO_PIN_SET(MDC_SDA0_PORT, MDC_SDA0_PIN, 0);
	}
}


void mdc_slot1_scl(int level)
{
	if(level){
		GPIOPinTypeGPIOInput(MDC_SCL0_PORT, MDC_SCL0_PIN);
		GPIO_PIN_SET(MDC_SCL0_PORT, MDC_SCL0_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(MDC_SCL0_PORT, MDC_SCL0_PIN);
		GPIO_PIN_SET(MDC_SCL0_PORT, MDC_SCL0_PIN, 0);
	}  
}


uint8 mdc_slot1_sda_in(void)
{
	 return (GPIO_ReadSinglePin(MDC_SDA0_PORT, MDC_SDA0_PIN));
}


void mdc_slot2_sda(int level)
{
	 if(level){
		GPIOPinTypeGPIOInput(MDC_SDA1_PORT, MDC_SDA1_PIN);
		GPIO_PIN_SET(MDC_SDA1_PORT, MDC_SDA1_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(MDC_SDA1_PORT, MDC_SDA1_PIN);
		GPIO_PIN_SET(MDC_SDA1_PORT, MDC_SDA1_PIN, 0);
	}
}


void mdc_slot2_scl(int level)
{
	if(level){
		GPIOPinTypeGPIOInput(MDC_SCL1_PORT, MDC_SCL1_PIN);
		GPIO_PIN_SET(MDC_SCL1_PORT, MDC_SCL1_PIN, 1);
	}else{
		GPIOPinTypeGPIOOutput(MDC_SCL1_PORT, MDC_SCL1_PIN);
		GPIO_PIN_SET(MDC_SCL1_PORT, MDC_SCL1_PIN, 0);
	}  
}


uint8 mdc_slot2_sda_in(void)
{
	 return (GPIO_ReadSinglePin(MDC_SDA1_PORT, MDC_SDA1_PIN));
}





