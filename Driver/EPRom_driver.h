#ifndef _EPROM_DRIVER_H_
#define _EPROM_DRIVER_H_




int eeprom_write(iic_device_t device, uint32 addr, uint8 *buf, size_t size);

int eeprom_read(iic_device_t device, uint32 addr, uint8 *buf, size_t size);

void epprom_sda(int level);


void epprom_scl(int level);


uint8 epprom_sda_in(void);



#endif











