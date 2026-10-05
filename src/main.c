#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <process.h>
#define PATH_DELIMITER ";"
#else
#include <sys/wait.h>
#define PATH_DELIMITER ":"
#endif

char *get_path(const char *cmd) {
  if (strchr(cmd, '/') != NULL
#ifdef _WIN32
      || strchr(cmd, '\\') != NULL
#endif
  ) {
    if (access(cmd, X_OK) == 0) {
      return strdup(cmd);
    }
    return NULL;
  }

  char *path_env = getenv("PATH");
  if (path_env == NULL) {
    return NULL;
  }

  char *path_copy = strdup(path_env);
  if (path_copy == NULL) {
    return NULL;
  }

  char *dir = strtok(path_copy, PATH_DELIMITER);
  while (dir != NULL) {
    char full_path[1024];
    snprintf(full_path, sizeof(full_path), "%s/%s", dir, cmd);

    if (access(full_path, X_OK) == 0) {
      char *result = strdup(full_path);
      free(path_copy);
      return result;
    }

    dir = strtok(NULL, PATH_DELIMITER);
  }

  free(path_copy);
  return NULL;
}

int handle_type(char *args[]) {
  if (args[1] == NULL) {
    return 0;
  }
  char *cmd = args[1];
  char *builtins[] = {"cd", "exit", "echo", "type", "pwd"};
  int num_builtins = (int)(sizeof(builtins) / sizeof(builtins[0]));

  // check builtins first
  for (int i = 0; i < num_builtins; i++) {
    if (strcmp(cmd, builtins[i]) == 0) {
      printf("%s is a shell builtin\n", cmd);
      return 0;
    }
  }

  // search PATH
  char *full_path = get_path(cmd);
  if (full_path != NULL) {
    printf("%s is %s\n", cmd, full_path);
    free(full_path);
    return 0;
  }

  printf("%s: not found\n", cmd);
  return 0;
}

int main(int argc, char *argv[]) {
  (void)argc;
  (void)argv;
  setbuf(stdout, NULL);
  char command[1024];
  // int startindex = -1;
  // int endindex = -1;

  while (1) {
    printf("$ ");

    if (fgets(command, sizeof(command), stdin) == NULL) {
      break;
    }

    command[strcspn(command, "\r\n")] = '\0';

    char *args[64];
    int arg_count = 0;
    char *src = command; // read pointer  — reads every character
    char *dst = command; // write pointer — writes only characters we keep

    while (*src != '\0' && arg_count < 63) {
      while (*src == ' ' || *src == '\t')
        src++;

      if (*src == '\0')
        break;

      args[arg_count++] = dst;

      while (*src != '\0' && *src != ' ' && *src != '\t') {
        if (*src == '\'' || *src == '\"') {
          char quote = *src;
          src++;

          while (*src != '\0' && *src != quote) {
            if (quote == '\"' && *src == '\\') {
              src++;
            }
            *dst++ = *src++;
          }
          if (*src == quote)
            src++;
        } else if (*src == '\\') {
          src++;
          if (*src != '\0') {
            *dst++ = *src++;
          }
        } else {

          *dst++ = *src++;
        }
      }

      if (*src != '\0')
        src++;

      *dst++ = '\0';
    }
    args[arg_count] = NULL;

    if (arg_count == 0) {
      continue;
    }

    char *output_file = NULL;
    char *error_file = NULL;
    int redirect_stdout = 0;
    int redirect_stderr = 0;

    for (int i = 0; args[i] != NULL; i++) {
      if (strcmp(args[i], ">") == 0 || strcmp(args[i], "1>") == 0) {
        if (args[i + 1] != NULL) {
          output_file = args[i + 1];
          redirect_stdout = 1;
          int j = i;
          while (args[j + 2] != NULL) {
            args[j] = args[j + 2];
            j++;
          }
          args[j] = NULL;
          args[j + 1] = NULL;
          i--;
          continue;
        }
      } else if (args[i][0] == '1' && args[i][1] == '>' && args[i][2] != '\0') {
        output_file = &args[i][2];
        redirect_stdout = 1;
        int j = i;
        while (args[j + 1] != NULL) {
          args[j] = args[j + 1];
          j++;
        }
        args[j] = NULL;
        i--;
        continue;
      } else if (args[i][0] == '>' && args[i][1] != '\0') {
        output_file = &args[i][1];
        redirect_stdout = 1;
        int j = i;
        while (args[j + 1] != NULL) {
          args[j] = args[j + 1];
          j++;
        }
        args[j] = NULL;
        i--;
        continue;
      }

      if (strcmp(args[i], "2>") == 0) {
        if (args[i + 1] != NULL) {
          error_file = args[i + 1];
          redirect_stderr = 1;
          int j = i;
          while (args[j + 2] != NULL) {
            args[j] = args[j + 2];
            j++;
          }
          args[j] = NULL;
          args[j + 1] = NULL;
          i--;
          continue;
        }
      } else if (args[i][0] == '2' && args[i][1] == '>' && args[i][2] != '\0') {
        error_file = &args[i][2];
        redirect_stderr = 1;
        int j = i;
        while (args[j + 1] != NULL) {
          args[j] = args[j + 1];
          j++;
        }
        args[j] = NULL;
        i--;
        continue;
      }
    }

    int saved_stdout = -1;
    int saved_stderr = -1;
    if (redirect_stdout && output_file != NULL) {
      int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd >= 0) {
        saved_stdout = dup(STDOUT_FILENO);
        dup2(fd, STDOUT_FILENO);
        close(fd);
      }
    }

    if (redirect_stderr && error_file != NULL) {
      int fd = open(error_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
      if (fd >= 0) {
        saved_stderr = dup(STDERR_FILENO);
        dup2(fd, STDERR_FILENO);
        close(fd);
      }
    }

    if (args[0] == NULL) {
      if (saved_stdout != -1) {
        fflush(stdout);
        dup2(saved_stdout, STDOUT_FILENO);
        close(saved_stdout);
      }
      if (saved_stderr != -1) {
        fflush(stderr);
        dup2(saved_stderr, STDERR_FILENO);
        close(saved_stderr);
      }
      continue;
    }

    if (strcmp(args[0], "exit") == 0) {
      int exit_code = 0;
      if (args[1] != NULL) {
        exit_code = atoi(args[1]);
      }
      if (saved_stdout != -1) {
        close(saved_stdout);
      }
      if (saved_stderr != -1) {
        close(saved_stderr);
      }
      exit(exit_code);
    } else if (strcmp(args[0], "echo") == 0) {
      for (int i = 1; args[i] != NULL; i++) {
        if (i > 1) {
          putchar(' ');
        }
        fputs(args[i], stdout);
      }
      putchar('\n');
    } else if (strcmp(args[0], "type") == 0) {
      handle_type(args);
    } else if (strcmp(args[0], "pwd") == 0) {
      char cwd[1024];
      if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s\n", cwd);
      }
    } else if (strcmp(args[0], "cd") == 0) {
      if (args[1] == NULL) {
        fprintf(stderr, "cd: missing argument\n");
      } else if (strcmp(args[1], "~") == 0) {
        char *home = getenv("HOME");
        if (home != NULL) {
          chdir(home);
        }
      } else {
        chdir(args[1]);
      }
    } else {
      char *exec_path = get_path(args[0]);
      if (exec_path != NULL) {
#ifdef _WIN32
        _spawnv(_P_WAIT, exec_path, (const char *const *)args);
#else
        pid_t pid = fork();
        if (pid == 0) {
          execv(exec_path, args);
          perror("execv");
          exit(EXIT_FAILURE);
        } else if (pid > 0) {
          int status;
          waitpid(pid, &status, 0);
        } else {
          perror("fork");
        }
#endif
        free(exec_path);
      } else {
        printf("%s: command not found\n", args[0]);
      }
    }

    if (saved_stdout != -1) {
      fflush(stdout);
      dup2(saved_stdout, STDOUT_FILENO);
      close(saved_stdout);
    }
    if (saved_stderr != -1) {
      fflush(stderr);
      dup2(saved_stderr, STDERR_FILENO);
      close(saved_stderr);
    }
  }

  return 0;
}
