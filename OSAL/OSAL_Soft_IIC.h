#ifndef _OSAL_SOFT_IIC_H_
#define _OSAL_SOFT_IIC_H_


typedef enum{
	PCM1795=0,
	SRC4382,
	EPROM,
	MDC_SLOT1_IIC,
	MDC_SLOT2_IIC,
	IIC_DEVICE_NUM
}iic_device_t;


#define IIC_HALF_DEALY_TIME  15
#define IIC_DELAY_TIME     30




int osal_iic_write_buf(iic_device_t device, uint32 map, uint8 *buf, size_t cnt);
int osal_iic_write_byte(iic_device_t device, uint32 map, uint8 data);

int osal_iic_read_buf(iic_device_t device, uint32 map, uint8 *buf, size_t cnt);


#endif







