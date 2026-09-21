#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>


#ifdef _WIN32
#include <process.h>
#define PATH_DELIMITER ";"
#else
#include <sys/wait.h>
#define PATH_DELIMITER ":"
#endif

char *get_path(const char *cmd)
{
    if (strchr(cmd, '/') != NULL
#ifdef _WIN32
        || strchr(cmd, '\\') != NULL
#endif
    )
    {
        if (access(cmd, X_OK) == 0)
        {
            return strdup(cmd);
        }
        return NULL;
    }

    char *path_env = getenv("PATH");
    if (path_env == NULL)
    {
        return NULL;
    }

    char *path_copy = strdup(path_env);
    if (path_copy == NULL)
    {
        return NULL;
    }

    char *dir = strtok(path_copy, PATH_DELIMITER);
    while (dir != NULL)
    {
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);

        if (access(full_path, X_OK) == 0)
        {
            char *result = strdup(full_path);
            free(path_copy);
            return result;
        }

        dir = strtok(NULL, PATH_DELIMITER);
    }

    free(path_copy);
    return NULL;
}

int handle_type(char *args[])
{
    if (args[1] == NULL)
    {
        return 0;
    }
    char *cmd = args[1];
    char *builtins[] = {"cd", "exit", "echo", "type", "pwd"};
    int num_builtins = (int)(sizeof(builtins) / sizeof(builtins[0]));

    // check builtins first
    for (int i = 0; i < num_builtins; i++)
    {
        if (strcmp(cmd, builtins[i]) == 0)
        {
            printf("%s is a shell builtin\n", cmd);
            return 0;
        }
    }

    // search PATH
    char *full_path = get_path(cmd);
    if (full_path != NULL)
    {
        printf("%s is %s\n", cmd, full_path);
        free(full_path);
        return 0;
    }

    printf("%s: not found\n", cmd);
    return 0;
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    setbuf(stdout, NULL);
    char command[1024];

    while (1)
    {
        printf("$ ");

        if (fgets(command, sizeof(command), stdin) == NULL)
        {
            break;
        }

        command[strcspn(command, "\n")] = '\0';

        // Parse command into arguments
        char *args[64];
        int arg_count = 0;
        char *token = strtok(command, " ");
        while (token != NULL && arg_count < 63)
        {
            args[arg_count++] = token;
            token = strtok(NULL, " ");
        }
        args[arg_count] = NULL;

        if (arg_count == 0)
        {
            continue;
        }

        if (strcmp(args[0], "exit") == 0)
        {
            int exit_code = 0;
            if (args[1] != NULL)
            {
                exit_code = atoi(args[1]);
            }
            exit(exit_code);
        }
        else if (strcmp(args[0], "echo") == 0)
        {
            for (int i = 1; args[i] != NULL; i++)
            {
                if (i > 1)
                {
                    putchar(' ');
                }
                fputs(args[i], stdout);
            }
            putchar('\n');
        }
        else if (strcmp(args[0], "type") == 0)
        {
            handle_type(args);
        }
        else if (strcmp(args[0], "pwd") == 0)
        {
            char cwd[1024];
            if (getcwd(cwd, sizeof(cwd)) != NULL)
            {
                printf("%s\n", cwd);
            }
        }
        else if(strcmp(args[0], "cd") == 0){
            if (args[1] == NULL)
            {
                fprintf(stderr, "cd: missing argument\n");
                continue;
            } 
            else if (strcmp(args[1], "~") == 0){
                char *home = getenv("HOME");
                if (home != NULL){
                    chdir(home);
                }
            }
            chdir(args[1]);
        }
        else
        {
            char *exec_path = get_path(args[0]);
            if (exec_path != NULL)
            {
#ifdef _WIN32
                _spawnv(_P_WAIT, exec_path, (const char *const *)args);
#else
                pid_t pid = fork();
                if (pid == 0)
                {
                    execv(exec_path, args);
                    perror("execv");
                    exit(EXIT_FAILURE);
                }
                else if (pid > 0)
                {
                    int status;
                    waitpid(pid, &status, 0);
                }
                else
                {
                    perror("fork");
                }
#endif
                free(exec_path);
            }
            else
            {
                printf("%s: command not found\n", args[0]);
            }
        }
    }

    return 0;
}