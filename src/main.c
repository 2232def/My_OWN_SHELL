#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

int main(int argc, char *argv[])
{
    setbuf(stdout, NULL);
    char command[1024];

    while (1)
    {
        printf("$ ");

        fgets(command, sizeof(command), stdin);

        command[strcspn(command, "\n")] = '\0';
        if (strcmp(command, "exit") == 0)
        {
            break;
        }
        else if (strncmp(command, "echo", 4) == 0)
        {
            char *args = command + 4;
            while (*args != '\0' && isspace((unsigned char)*args))
            {
                args++;
            }

            int in_space = 0;
            while (*args != '\0')
            {
                if (isspace((unsigned char)*args))
                {
                    in_space = 1;
                }
                else
                {
                    if (in_space)
                    {
                        putchar(' ');
                        in_space = 0;
                    }
                    putchar(*args);
                }
                args++;
            }
            putchar('\n');
        }
        else
        {
            printf("%s: command not found\n", command);
        }
    }

    return 0;
}