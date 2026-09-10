#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char command[1024];

    printf("$ ");

    fgets(command, sizeof(command), stdin);

    command[strcspn(command, "\n")] = '\0';
    printf("%s: command not found\n", command);

    return 0;
}