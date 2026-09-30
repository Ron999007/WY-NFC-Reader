#include "mid_shell.h"

int shell_str_len(char *str)
{
    int i = 0;
    while(str[i++] != 0);
    return i - 1;
}

int shell_str_cmp(char *str1, char *str2, int len1, int len2)
{
    int i;
    if(len1 > len2) return 1;

    for(i = 0; i < len1; i++)
    {
        if(str1[i] != str2[i]) return 2;
    }

    /* Make sure we matched a whole command, and not only a substring */
    if(len2 > len1 && str2[i] != ' ')
    {
        return 2;
    }
    return 0;
}

int shell_parse_int(char *str)
{
    int val = 0;
    int i   = 0;
    char c;

    while((c = str[i++]) != 0)
    {
        if(c < '0' || c > '9') 
        {
            val = -1;
            break;
        }
        val = val * 10 + (c - '0');
    }
    return val;
}

int shell_arg_parser(char *cmd_line, int len, shell_cmd_args *args)
{
    int i, j;
    int spos = 0;
    int argc = 0;

    for(i = 0; i < len; i++)
    {
        if(argc > SHELL_MAX_ARGS) return 1; /* Too many arguments */

        if(cmd_line[i] == ' ' || i == len - 1)
        {
            if(i == len - 1) i++; /* Catch the last argument */

            if(spos == 0)
            {
                spos = i; /* Ignore first since it is the command itself */
            }
            else
            {
                if(i - spos > SHELL_MAX_ARG_LEN) return 2; /* Argument value too long */

                for(j = 0; j < i - spos - 1; j++)
                {
                    args->args[argc].val[j] = cmd_line[spos + 1 + j];
                }
                args->args[argc++].val[j] = 0;
                spos = i;
            }
        }
    }
    args->count = argc;
    return 0;
}

int shell_process_cmds(shell_cmds *cmds, char *cmd_line)
{
    int i, ret, cmd_len, cmd_line_len;
    shell_cmd_args args;

    for(i = 0; i < cmds->count; i++)
    {
        cmd_line_len = shell_str_len(cmd_line);
        cmd_len      = shell_str_len((char *)(cmds->cmds[i].cmd));

        if(shell_str_cmp((char *)(cmds->cmds[i].cmd), cmd_line, cmd_len, cmd_line_len) == 0)
        {
            ret = shell_arg_parser(cmd_line, cmd_line_len, &args);

            if(ret == 1) return SHELL_PROCESS_ERR_ARGS_MAX;
            if(ret == 2) return SHELL_PROCESS_ERR_ARGS_LEN;

            return (cmds->cmds[i].func)(&args);
        }
    }
    return SHELL_PROCESS_ERR_CMD_UNKN;
}