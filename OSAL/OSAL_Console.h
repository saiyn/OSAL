#ifndef _OSAL_CONSOLE_H_
#define _OSAL_CONSOLE_H_


typedef enum{
	CMD_NO_ERROR=0,
	CMD_UNKNOWN,
	CMD_PARAMETER_ERROR
}cmd_err_t;

struct cmd_tbl_s{
	char *name; /*command name*/
	int maxargs; /*maximum number of the arguments*/
	int (*cmd)(struct cmd_tbl_s *, int, char * const[]);
	char *usage; /*usgae message*/
};

typedef struct cmd_tbl_s cmd_tbl_t;


#define OSAL_CMD(_name, _maxargs, _cmd, _usage) \
			const cmd_tbl_t __init_cmd_##_name  __attribute__((used, section("INIT_CMD"))) = \
       {#_name, _maxargs, _cmd, _usage}


#define is_blank(c)  (c == ' ' || c == '\t')

cmd_err_t osal_run_command(char *cmd);

void Init_all(unsigned long addr,unsigned long len);

#endif







