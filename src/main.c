#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

int handle_type(char *args[])
{
    if (args[1] == NULL) {
        return 0;
    }
    char *cmd = args[1];
    char *builtins[] = {"cd", "exit", "echo", "type"};
    
    int a = ((char*)(&builtins + 1) - (char*)(&builtins)); 
    int b = (char*)(builtins + 1) - (char*)builtins;

    int num_builtins = a/b;
    // check builtins first
    for (int i = 0; i < num_builtins; i++)
    {
        if (strcmp(cmd, builtins[i]) == 0)
        {
            printf("%s is a shell builtin\n", cmd);
            return;
        }
    }

    // not a builtin — search PATH
    char *path_env = getenv("PATH");
    if (path_env == NULL)
    {
        printf("%s: not found\n", cmd);
        return;
    }

    char *path_copy = strdup(path_env);
    char *dir = strtok(path_copy, ":");

    while (dir != NULL)
    {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);

        if (access(full_path, X_OK) == 0)
        {
            printf("%s is %s\n", cmd, full_path);
            free(path_copy);
            return;
        }

        dir = strtok(NULL, ":");
    }

    printf("%s: not found\n", cmd);
    free(path_copy);
}

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
        else if (strncmp(command, "type", 4) == 0)
        {
            char *args[64];
            char *copy = strdup(command);
            int i = 0;
            char *token = strtok(copy, " ");
            while (token != NULL && i < 63) {
                args[i++] = token;
                token = strtok(NULL, " ");
            }
            args[i] = NULL;

            handle_type(args);
        }
        else
        {
            printf("%s: command not found\n", command);
        }
    }

    return 0;
}