#ifndef _SYS_CONFIG_H_
#define _SYS_CONFIG_H_

//#define  NAD_C358 
//#define  NAD_C378 


#define CONFIG_USE_CONSOLE

#define MDC_USB_UPGRADE_TIMEOUT  (10*3) //3S


#define EPPROM_IIC_ADDRESS 0xA0

#define AUTO_STANDBY_TIMEOUT  (20*60*1000ul)

#define DATABASE_SAVE_TIMEOUT  (1000)

#define VOL_MENU_3S_TIMEOUT  (3 * 1000)
#define DIS_TIMEOUT          (20 * 1000)

#define SYS_CONFIG_BACK_TIMEOUT  (20 * 1000)

#define SYS_NV_ADDRESS  0x00
#define SYS_LOW_POWER_FLAG_ADDRESS 0x200
#define SYS_NV_ADDRESS_MDC  0x400
#define SYS_NV_ADDRESS_EQ   0x800

#define CONFIG_SYS_MAXARGS  16


//#define MASTER_VOL_INDEX_MAX     10.0
#define MASTER_VOL_INDEX_MAX     15.0
#define MASTER_VOL_INDEX_MIN     (-70.0)
//#define NJW1194_0DB_GAIN_VALUE   0x36  		//+5db

#define NJW1194_0DB_GAIN_VALUE   0x40  		//+0db

#define MASTER_VOL_WIDTH       (160)

#define MASTER_VOL_WIDTH_INT    (80)

#define MASTER_VOL_INDEX_MIN_INT  (-70)

#define SYS_TREBLE_VOL_MIN    (-7)
#define SYS_TREBLE_VOL_MAX    (7)
#define SYS_BASS_VOL_MIN      (-7)
#define SYS_BASS_VOL_MAX      (7)
#define SYS_BALANCE_MIN       (-7)



#define VOL_MENU_DIS_TIMEOUT   (1000*1)

#define MSG_MAX_LEN 128

#define SUB_DELAY_MAX 250

#define DEFAULTT_SOURCE        RCA 

#define POWER_ON_METHOD_DEFAULT   AUDIO

#define DSP_DELAY_DEFAULT_ROUTER  DELAY_OFF


#define SYSTICK_INT_PRIORITY    0x80



#endif


