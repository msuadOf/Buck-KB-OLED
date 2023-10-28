#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "usart.h"
int UART_printf(UART_HandleTypeDef *huart, const char *fmt, ...);
#define  printf(...) UART_printf(&huart1,__VA_ARGS__)

static int cmd_help(char *args);


extern int cmd_setPID(char *args);
extern int cmd_systemctl(char *args);
extern int cmd_setSampleRate(char *args);

static struct
{
    const char *name;
    const char *description;
    int (*handler)(char *);
} cmd_table[] = {
        {"help", "Display information about all supported commands", cmd_help},
        {"setPID", "Usage: setPID kp ki kd (Vset)", cmd_setPID},
        {"systemctl", "Usage: systemctl enable/disable <func>", cmd_systemctl},
        {"setSampleRate", "Usage: systemctl enable/disable <func>", cmd_setSampleRate},

        /* TODO: Add more commands */

};

#define ARRLEN(arr) (int)(sizeof(arr) / sizeof(arr[0]))
#define NR_CMD ARRLEN(cmd_table)



static int cmd_help(char *args)
{
    /* extract the first argument */
    char *arg = strtok(NULL, " ");
    int i;

    if (arg == NULL)
    {
        /* no argument given */
        for (i = 0; i < NR_CMD; i++)
        {
            printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
        }
    }
    else
    {
        for (i = 0; i < NR_CMD; i++)
        {
            if (strcmp(arg, cmd_table[i].name) == 0)
            {
                printf("%s - %s\n", cmd_table[i].name, cmd_table[i].description);
                return 0;
            }
        }
        printf("Unknown command '%s'\n", arg);
    }
    return 0;
}

int UART_Console(char *str)
{

    printf("\n> ");
        char *str_end = str + strlen(str);

        /* extract the first token as the command */
        char *cmd = strtok(str, " ");
        if (cmd == NULL)
        {
            printf("You entered nothing!\n");
        }

        /* treat the remaining string as the arguments,
         * which may need further parsing
         */
        char *args = cmd + strlen(cmd) + 1;
        if (args >= str_end)
        {
            args = NULL;
        }

        int i;
        for (i = 0; i < NR_CMD; i++)
        {
            if (strcmp(cmd, cmd_table[i].name) == 0)
            {
                if (cmd_table[i].handler(args) < 0)
                {
                    printf("command %s has execute failure\n",cmd);
                    return -1;
                }
                break;
            }
        }

        if (i == NR_CMD)
        {
            printf("Unknown command '%s'\n", cmd);
        }

    printf("\n");
    return 0;
}

