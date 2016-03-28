#include "sys_head.h"
#include "OSAL_RingBuf.h"

extern ring_buffer_t ir_in;
extern ring_buffer_t ir_front;
extern ring_buffer_t console;
extern ring_buffer_t nad_app;
extern ring_buffer_t bt_uart;
extern ring_buffer_t mdc_slot1;
extern ring_buffer_t mdc_slot2;

ring_buffer_t *ring_list[] ={
	&console,
	&bt_uart,
	&nad_app,
	&ir_front,
	&ir_in,
	&mdc_slot1,
	&mdc_slot2
};

#define RING_BUF_COUNT (sizeof(ring_list)/sizeof(ring_list[0]))

void ring_buffer_init(void)
{
	 uint8 index;
	
	 for(index = 0; index < RING_BUF_COUNT; index++)
	{
		   ring_list[index]->read = 0;
		   ring_list[index]->write = 0;
		   memset(ring_list[index]->buffer, 0, sizeof(ring_list[index]->buffer));
		  
	}

}

void ring_buffer_write(ringbuf_device_t index, uint8 ch)
{
	  ring_buffer_t *p;
	  uint8 next = 0;
	
	  OSAL_ASSERT(index < DEVICE_NUM);
	
	  p = ring_list[index];
	
	  next = (p->write + 1) % RING_BUF_SIZE;
	
	  /*ring buf is full*/
	  if(p->read == next) return;
	
	  p->buffer[p->write] = ch;
	
	  p->write = next;
}


uint8 ring_buffer_read(ringbuf_device_t index)
{  
	  uint8 byte = 0;
	  ring_buffer_t *p;
	
	  OSAL_ASSERT(index < DEVICE_NUM);
	
	  p = ring_list[index];
	
	  /*the buffer is empty, just return 0*/
	  if(p->read == p->write) return 0;
	
	  byte = p->buffer[p->read];
		
	  p->read = (p->read + 1) % RING_BUF_SIZE;
	
		return byte;
}

bool Is_ring_buffer_empty(ringbuf_device_t index)
{
	  ring_buffer_t *p;
	
	  OSAL_ASSERT(index < DEVICE_NUM);
	
	  p = ring_list[index];
	
	  return (p->read == p->write);
}

size_t ring_buffer_len(ringbuf_device_t index)
{
	  ring_buffer_t *p;
	  OSAL_ASSERT(index < DEVICE_NUM);
	
	  p = ring_list[index];
	
    if(p->write >= p->read) return (p->write - p->read);
	  else return(RING_BUF_SIZE + p->write - p->read);
}



size_t ring_buffer_peek_read(ringbuf_device_t index, uint8 *buf, size_t len)
{
	ring_buffer_t *p;
	uint8 read;
	uint8 size;
	uint8 j;
	uint8 l;
	
	OSAL_ASSERT(buf != NULL);
	OSAL_ASSERT(index < DEVICE_NUM);
	
	p = ring_list[index];
	read = p->read;
	size =  ring_buffer_len(index);
	
	l = ((len >= size) ? size : len);
	
	for(j = 0; j < l; j++){
		*buf++ = p->buffer[read];
		read = (read + 1) % RING_BUF_SIZE;
	}
	
	return l;
}




