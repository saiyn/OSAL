/*
* Copyright (c) 2015, Hansong (Nanjing) Technology Ltd.
* All rights reserved.
* 
* ----File Info ----------------------------------------------- -----------------------------------------
* Name:     OSAL_Port.c
* Author:   saiyn.chen
* Date:     2015/3/31
* Summary:  this file is where to do bsp port.


* Change Logs:
* Date           Author         Notes
* 2015-03-31     saiyn.chen     first implementation
*/
#include "sys_head.h"
#include "OSAL_Config.h"
#include "OSAL.h"
#include "OSAL_Task.h"



#ifdef CONFIG_USE_ENV

#include "OSAL_Env.h"
#define NV_PAGE_SIZE  1024
#define ENV_START_ADDR 0x801EC00
#define ENV_REASE_MIN_SIZE (NV_PAGE_SIZE)
#define ENV_SECTION_SIZE   (NV_PAGE_SIZE / 4)

const static env_obj_t default_env_set[] = {
	  {"in factory", "yes"},
    {"source", "aux"}
};

bool log_on_nad = false;


void osal_nv_init(uint32 *start_addr, size_t *total_size, size_t *erase_min_size, 
          const env_obj_t **default_env, size_t *default_size) {
						 
	*start_addr = ENV_START_ADDR;
	*total_size = ENV_SECTION_SIZE;					 
	*erase_min_size = ENV_REASE_MIN_SIZE;
	*default_env = default_env_set;						
	*default_size = sizeof(default_env_set) / sizeof(default_env_set[0]);					 						 
}

static env_err_t flash_erase(uint32 addr, size_t size)
{
	 env_err_t result = ENV_NO_ERR;
 	
	 return result;
}

env_err_t nv_read(uint32 addr, uint32 *buf, size_t size)
{
	  env_err_t result = ENV_NO_ERR;
	  
	  OSAL_ASSERT(size >= 4);
		OSAL_ASSERT(size % 4 == 0);
	
		for(;size > 0; size -= 4, addr +=4, buf++){
			*buf = *(uint32 *)addr;
		}
		
		return result;
}

env_err_t nv_write(uint32 addr, uint32 *buf, size_t size)
{
	 env_err_t result = ENV_NO_ERR; 
	 OSAL_ASSERT(size >= 4);
	 OSAL_ASSERT(size % 4 == 0);
	 OSAL_ASSERT(buf);
	
	 result = flash_erase(addr, size);
	
	 
	 return result;
}

void s_printf(char *fmt, ...)
{
	    va_list ap;
	    char buf[512] = {0};
      size_t len;
      size_t index = 0;

      va_start(ap, fmt);
	    vsprintf(buf, fmt, ap);
      va_end(ap);

      len = strlen(buf);
      while(index < len){
				if(log_on_nad == true){
					while(UARTCharPutNonBlocking(UART3_BASE, buf[index]) == false);
				}else{
					while(UARTCharPutNonBlocking(UART0_BASE, buf[index]) == false);
				}
				index++;
			}
}



#endif









