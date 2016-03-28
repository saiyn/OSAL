#ifndef _OSAL_ENV_H_
#define _OSAL_ENV_H_

#if 1
#define ENV_LOG  SYS_TRACE
#else
#define ENV_LOG(...)
#endif



typedef enum
{
	ENV_NO_ERR=0,
	ENV_ERASE_ERR,
	ENV_WRITE_ERR,
}env_err_t;

typedef struct
{
	char *name;
	char *value;
}env_obj_t;



env_err_t osal_env_init(void);

char *osal_env_get(const char *key);

env_err_t osal_env_set(const char *key, const char *value);

env_err_t osal_env_save(void);

env_err_t osal_set_default_env(void);

void osal_print_env(void);

#endif


