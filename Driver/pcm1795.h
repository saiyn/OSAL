#ifndef _PCM_1795_H_
#define _PCM_1795_H_


#define PCM1795_IIC_ADDRESS   0x98

#define PCM1795_RST_H   GPIO_PIN_SET(PCM_1795_RST_PORT, PCM_1795_RST_PIN, 1)
#define PCM1795_RST_L   GPIO_PIN_SET(PCM_1795_RST_PORT, PCM_1795_RST_PIN, 0)

#define PCM1795_REG_20  0x14

void pcm1795_sda(int level);

void pcm1795_scl(int level);

uint8 pcm1795_sda_in(void);


int pcm1795_init(void);

#endif


