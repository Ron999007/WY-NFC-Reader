#ifndef __MID_SHELL_H__
#define __MID_SHELL_H__

/* Maximum number of characters for a command line passed to the shell */
#define SHELL_MAX_CMD_LINE  90  

/* Maximum number of arguments passed with one command to the shell */
#define SHELL_MAX_ARGS      10

/* Maximum number of characters for a single argument */
#define SHELL_MAX_ARG_LEN   40

/* Return code given when processing of a command line was OK */
#define SHELL_PROCESS_OK    0

/* ERROR: Maximum number of arguments was reached */
#define SHELL_PROCESS_ERR_ARGS_MAX  0xFFF0

/* ERROR: Maximum number of chars for an argument was reached */
#define SHELL_PROCESS_ERR_ARGS_LEN  0xFFF1

/* ERROR: Unknown command */
#define SHELL_PROCESS_ERR_CMD_UNKN  0xFFF2

/* Single command argument structure */
typedef struct {
    char val[SHELL_MAX_ARG_LEN];
} shell_cmd_arg;

/* All arguments from a single command line */
typedef struct {
    unsigned char count;
    shell_cmd_arg args[SHELL_MAX_ARGS];
} shell_cmd_args;

/* Definition of a single shell command */
typedef struct {
    const char *cmd;                   /* Name of the command */
    const char *desc;                  /* Description of the command */
    int (*func)(shell_cmd_args *args); /* Callback function */
} shell_cmd;

/* All shell commands known by the shell (Refactored to use a pointer) */
typedef struct {
    unsigned char count;       /* Number of commands in the table */
    const shell_cmd *cmds;     /* Pointer to the command table array */
} shell_cmds;

/* API Declarations */
int shell_str_len(char *str);
int shell_str_cmp(char *str1, char *str2, int len1, int len2);
int shell_parse_int(char *str);
int shell_arg_parser(char *cmd_line, int len, shell_cmd_args *args);
int shell_process_cmds(shell_cmds *cmds, char *cmd_line);

#endif /* __MID_SHELL_H__ */