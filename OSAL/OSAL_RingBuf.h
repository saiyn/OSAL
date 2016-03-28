#ifndef _OSAL_RINGBUF_H_
#define _OSAL_RINGBUF_H_


#define RING_BUF_SIZE  512


typedef struct
{
	   uint8  buffer[RING_BUF_SIZE];
	   //size_t bytes_in_buffer;
	   uint8  read;
	   uint8  write;
}ring_buffer_t;


typedef  enum{
	CONSOLE =0,
	BT_UART,
	NAD_APP,
	IR_FRONT,
	IR_BACK_IN,
	MDC_SLOT1,
	MDC_SLOT2,
	DEVICE_NUM
}ringbuf_device_t;



void ring_buffer_init(void);

void ring_buffer_write(ringbuf_device_t index, uint8 ch);

size_t ring_buffer_len(ringbuf_device_t index);

bool Is_ring_buffer_empty(ringbuf_device_t index);

uint8 ring_buffer_read(ringbuf_device_t index);

size_t ring_buffer_peek_read(ringbuf_device_t index, uint8 *buf, size_t len);
#endif

