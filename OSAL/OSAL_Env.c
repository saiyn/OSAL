/*
* Copyright (c) 2015, Hansong (Nanjing) Technology Ltd.
* All rights reserved.
* 
* ----File Info ----------------------------------------------- -----------------------------------------
* Name:     OSAL_Env.c
* Author:   saiyn.chen
* Date:     2015/3/31
* Summary:  this file handle environment variables for the system.


* Change Logs:
* Date           Author         Notes
* 2015-03-31     saiyn.chen     first implementation
*/
#include "sys_head.h"
#include "OSAL.h"
#include "OSAL_Task.h"
#include "OSAL_Env.h"
#include "OSAL_Config.h"
#include "OSAL_Utility.h"

/*
*The enviornment variables system has two sections:
*
*1.env system information header section
*2.env system data body section
*/
enum {
	ENV_INDEX_END_ADDR=0,
#ifdef CONFIG_ENV_USE_CRC
	CRC_INDEX_IN_CACHE,
#endif
	ENV_SYSTEM_HEADER_WORD_SIZE
};
#define ENV_SYSTEM_HEADER_BYTE_SIZE  (ENV_SYSTEM_HEADER_WORD_SIZE * 4)


/*static global parameters for the environment system*/
static uint32 env_start_addr = 0;
static const env_obj_t *default_env_set = NULL;
static size_t default_env_size = 0;
static size_t env_total_size = 0;
static uint32 *env_cache = NULL;
static size_t erase_min_size =0;


static env_err_t  osal_env_load(void);
static env_err_t append_env(const char *key, const char *value);

static uint32 get_env_sys_addr(void)
{
	 return env_start_addr;
}

static uint32 get_env_body_start_addr(void)
{
	  return (env_start_addr + ENV_SYSTEM_HEADER_BYTE_SIZE);
}


static void set_env_end_addr(uint32 addr)
{
	 OSAL_ASSERT(env_cache);
	
	 env_cache[ENV_INDEX_END_ADDR] = addr;
}

static uint32 get_env_end_addr(void)
{
	  OSAL_ASSERT(env_cache);
	
	  return env_cache[ENV_INDEX_END_ADDR];
}

static size_t get_env_body_size(void)
{
	  return get_env_end_addr() - get_env_body_start_addr();
}

static size_t get_env_uesd_size(void)
{
	 return get_env_body_size() + ENV_SYSTEM_HEADER_BYTE_SIZE;
}


#ifdef CONFIG_ENV_USE_CRC
static void set_env_crc(uint32 crc)
{
	 OSAL_ASSERT(env_cache);

   env_cache[CRC_INDEX_IN_CACHE] = crc;
}

static uint32 get_env_crc(void)
{
	 OSAL_ASSERT(env_cache);

   return env_cache[CRC_INDEX_IN_CACHE];	
}

static uint32 calc_env_crc(void)
{
	 uint32 crc32 = 0;
	
	 crc32 = calc_crc32(crc32, &env_cache[ENV_INDEX_END_ADDR], 4);
	 crc32 = calc_crc32(crc32, &env_cache[ENV_SYSTEM_HEADER_WORD_SIZE], get_env_body_size());
	
	 ENV_LOG("calculate env crc32 value=0X%08x\r\n", crc32); 
	
	 return crc32;
}

static bool env_crc_is_ok(void)
{
	 if(calc_env_crc() == get_env_crc()) return true;
	 else return false;
	  
}

#endif

/*************************************************************************
*  @fn      osal_env_init
*  
*  @brief   This function initialize the env system.
*           
*  @param   none
*
*  @return  err code
*/
env_err_t osal_env_init(void)
{
	 extern void osal_nv_init(uint32 *start_addr, size_t *total_size, size_t *erase_min_size, 
          const env_obj_t **default_env, size_t *default_size);
	
	 env_err_t result = ENV_NO_ERR;
	
	 osal_nv_init(&env_start_addr, &env_total_size, &erase_min_size, &default_env_set, &default_env_size);
	 
	 OSAL_ASSERT(env_start_addr);
	 OSAL_ASSERT(env_total_size);
	 /*must be word aligned for environment variables*/
	 OSAL_ASSERT(env_total_size % 4 == 0);
	 OSAL_ASSERT(erase_min_size);
	 OSAL_ASSERT(default_env_set);
	 OSAL_ASSERT(default_env_size);
	 OSAL_ASSERT(default_env_size < env_total_size);
	 
	 ENV_LOG("Env start at 0X%08x and contain total %d bytes\r\n", env_start_addr, env_total_size);
	
	 /*create env cache */
	 env_cache = (uint32 *)MALLOC(sizeof(uint8) * env_total_size);
	 OSAL_ASSERT(env_cache);
	 
	 ENV_LOG("env_cache locat at %p\r\n", env_cache);
	 
	 result = osal_env_load();
	 
	 return result;
}

/*************************************************************************
*  @fn      osal_env_load
*  
*  @brief   This function load data from nv to RAM.
*           
*  @param   none
*
*  @return  err code
*/
static env_err_t  osal_env_load(void)
{
	extern env_err_t nv_read(uint32 addr, uint32 *buf, size_t size);
	
	env_err_t result;
	uint32 env_end_addr;
	uint32 *env_cache_bak;
#ifdef CONFIG_ENV_USE_CRC
	uint32 crc;
#endif
	
	OSAL_ASSERT(env_cache);
	
	result = nv_read(get_env_sys_addr() + ENV_INDEX_END_ADDR*4, &env_end_addr, 4);
 	OSAL_ASSERT(result == ENV_NO_ERR);
	
	if(env_end_addr == 0xFFFFFFFF || env_end_addr > env_start_addr + env_total_size)
	{
		 ENV_LOG("no valid data has been saved, will set default env\r\n");
		 result = osal_set_default_env();
	}
	else
	{
		 set_env_end_addr(env_end_addr); 
		 
		 env_cache_bak = env_cache + ENV_SYSTEM_HEADER_WORD_SIZE;
		
		 result = nv_read(get_env_body_start_addr(), env_cache_bak, get_env_body_size());
		 OSAL_ASSERT(result == ENV_NO_ERR);
		
		 ENV_LOG("env system load %d bytes data\r\n", get_env_body_size());
		
#ifdef CONFIG_ENV_USE_CRC
		 /*first load the crc value from the nv*/
		 result = nv_read(get_env_sys_addr() + CRC_INDEX_IN_CACHE*4, &crc, 4);
		 OSAL_ASSERT(result == ENV_NO_ERR);
		
		 set_env_crc(crc);
		
		 if(!env_crc_is_ok())
		 {
			   ENV_LOG("env crc[0X%08x] check fail and will set default env\r\n", crc);
			   result = osal_set_default_env();
		 }
		 else{
			   ENV_LOG("env crc check pass\r\n");
		 }
		
#endif
		 
	}
  
	return result;
}

/*************************************************************************
*  @fn      find_nev
*  
*  @brief   Check whether the given key is exist in the env.
*
*  @param   key - environment variable name
*           
*  @return  pointer of the found env or NULL if not found
*/
static char *find_env(const char *key)
{
	  char *env_start, *env_end;
	  char *found = NULL;
	  char *p = NULL;
	
	  OSAL_ASSERT(env_cache);
	
	  env_start = (char *)((char *)env_cache + ENV_SYSTEM_HEADER_BYTE_SIZE);
	  env_end = (char *)(env_start + get_env_body_size());
	
	  p = env_start;
	
	  while(p < env_end)
		{
			 found = strstr(p, key);
			 if(found && p[strlen(key)] == '='){
				
			 break;
			 }
			 
			p += WORD_SIZE_ALIGN(strlen(p) + 1);
		}
		
		if(p == env_end) return NULL;
		
		return found;
}


/*************************************************************************
*  @fn      delete_then_append_env
*  
*  @brief   If value is empty then just delete without append.
*  @param   key   - environment variable name
*           value - environment variable value
*
*  @return  err code
*/
static env_err_t delete_then_append_env(const char *key, const char *value)
{
	 env_err_t result = ENV_NO_ERR;
	 char *env;
	 size_t len;
	 size_t left;
	
	 env = find_env(key);
	 OSAL_ASSERT(env);
	
	 /*make sure WORD aligned*/
	 len = WORD_SIZE_ALIGN(strlen(env) + 1);
	 ENV_LOG("will delete %d bytes data->%s\r\n", len, env);
	 
	 /*calculate how many bytes data left need to be moved forward*/
	 left = get_env_body_size() - (size_t)((char *)(env + len) - (char *)(env_cache + ENV_SYSTEM_HEADER_WORD_SIZE));
	 if(left){
		   ENV_LOG("will move %d bytes from %p to %p\r\n", left, env + len, env);
		   memmove(env, (void *)(env + len), left);
	 }
	 else{
		 /*get here means the key is the last one in the env system*/
		 ENV_LOG("the last key will be deleted\r\n");
		 memset(env, 0, len);
	 }
	
	 /*update the env end addr to make sure the function get_env_body_size() return correct value*/
	 set_env_end_addr(get_env_end_addr() - len);
	 
	 if(value)
	 {
		  result = append_env(key,value);
	 }
	 
	
	 return result;
}

/*************************************************************************
*  @fn      append_env
*  
*  @brief   Append an enviornment variable.
*           
*  @param   key   - nevironment variable name
*           value - nevironment variable value
*
*  @return  err code
*/
static env_err_t append_env(const char *key, const char *value)
{
	 env_err_t result = ENV_NO_ERR;
	 char *env;
	 size_t len;
	
	 if(value)
	 {
		  /*'=' + '\0' => 2*/
		  len = strlen(key) + strlen(value) + 2;
		  len = WORD_SIZE_ALIGN(len);
		  ENV_LOG("will append %d bytes data\r\n", len);
		  env = (char *)MALLOC(len);
		  OSAL_ASSERT(env);
		 
		  sprintf(env, "%s=%s", key, value);
		  ENV_LOG("will append at %p\r\n", (char *)env_cache + get_env_uesd_size());
		  memcpy((char *)((char *)env_cache + get_env_uesd_size()), env, len);
		  FREE(env);
		  /*after append a new environment variable, we need update the env end addr again*/
	    set_env_end_addr(get_env_end_addr() + len); 
	 }
	 
	 return result;
}


/*************************************************************************
*  @fn      osal_env_set
*  
*  @brief   Set an enviornment variable. If value is empty, delect it.
*           If no key set before then add it.
*  @param   key   - nevironment variable name
*           value - nevironment variable value
*
*  @return  err code
*/
env_err_t osal_env_set(const char *key, const char *value)
{
	  env_err_t result = ENV_NO_ERR;

	  if(key){
		 if(find_env(key)){
			 result = delete_then_append_env(key,value);
		 }
		 else{
			 result = append_env(key,value);
		 }
		}
		
	return result;
}

/*************************************************************************
*  @fn      osal_env_get
*  
*  @brief   Get an environment variable by the key.
*  @param   key   - nevironment variable name
*           
*
*  @return  pointer to the value
*/
char *osal_env_get(const char *key)
{
	 char *found = NULL;
	 OSAL_ASSERT(env_cache);
	
	 if(key){
		 found = find_env(key);
		 if(found){
			 return &found[strlen(key)+1];
		 }
	 }
	 
	 return found;
}


/*************************************************************************
*  @fn      osal_env_save
*  
*  @brief   Save all environmnet variables from RAM to nv.
*  @param   none
*           
*
*  @return  err code
*/
env_err_t osal_env_save(void)
{
	 extern env_err_t nv_write(uint32 addr, uint32 *buf, size_t size);
	
	 env_err_t result = ENV_NO_ERR;
	
	 OSAL_ASSERT(env_cache);
	
#ifdef CONFIG_ENV_USE_CRC
	 env_cache[CRC_INDEX_IN_CACHE] =  calc_env_crc();
#endif
	
	 result = nv_write(get_env_sys_addr(), env_cache, get_env_uesd_size());
	
	 return result;
}

/*************************************************************************
*  @fn      osal_set_default_env
*  
*  @brief   Set default environment variables.
*  @param   none
*           
*
*  @return  err code
*/
env_err_t osal_set_default_env(void)
{
	size_t i;
	env_err_t result = ENV_NO_ERR;
	
	OSAL_ASSERT(env_cache);
	OSAL_ASSERT(default_env_set);
	OSAL_ASSERT(default_env_size);
	
	/*since no data has been saved, set the env end addr to the start addr of the data section*/
	set_env_end_addr(get_env_body_start_addr());
	
	for(i = 0; i < default_env_size; i++){
		osal_env_set(default_env_set[i].name, default_env_set[i].value);
	}
	
	osal_env_save();
	
	return result;
}

/*************************************************************************
*  @fn      print_env_data_section
*  
*  @brief   help function.
*  @param   none
*           
*  @return  none
*/
static void print_env_data_section(void)
{
	char *env_start, *env_end;
	char *p; 
	
	OSAL_ASSERT(env_cache);
	
	env_start = (char *)((char *)env_cache + ENV_SYSTEM_HEADER_BYTE_SIZE);
	env_end = (char *)(env_start + get_env_body_size());
	
	p = env_start;
	
	while(p < env_end){
		SYS_TRACE("%s\r\n", p);
		p += WORD_SIZE_ALIGN(strlen(p) + 1);
	}
}

/*************************************************************************
*  @fn      osal_print_env
*  
*  @brief   Print all environment variables.
*  @param   none
*           
*  @return  none
*/
void osal_print_env(void)
{	
	OSAL_ASSERT(env_cache);
	
	SYS_TRACE("\r\n*****************************************\r\n");
	SYS_TRACE("Header Section:\r\n");
	SYS_TRACE("Env system start address->0X%08x\r\n", get_env_sys_addr());
#ifdef CONFIG_ENV_USE_CRC
	SYS_TRACE("Env CRC->0X%08x\r\n", env_cache[CRC_INDEX_IN_CACHE]);
#endif 
	SYS_TRACE("Data Section:\r\n");
	print_env_data_section();
	SYS_TRACE("\r\n*****************************************\r\n");
}







