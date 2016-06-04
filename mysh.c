#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MAX_SIZE   1 << 10
#define DELIMS     " \t\r\n"

#define REDIRECT   ">"
#define APPEND     ">>"
#define BACKGROUND "&"

size_t parse(char*, char**);
bool lookupBuildInCommand(const char*);
void execute(char*);

int main(int argc, char** argv, char** env)
{
  char line[MAX_SIZE];

  while(true)
  {
    fprintf(stdout, "[%s]$", get_current_dir_name());
    if(!fgets(line, sizeof(line) - 1, stdin)) break;
    if(strcmp(line, "\n") == 0) continue;

    execute(line);
  }

  return EXIT_SUCCESS;
}

size_t parse(char* line, char** args)
{
  size_t n = 0;

  char* arg = strtok(line, DELIMS);

  while(arg != NULL)
  {
    args[n++] = arg;

    arg = strtok(NULL, DELIMS);
  }

  arg[n] = NULL;

  return n;
}

bool lookupBuildInCommand(const char* command)
{
  const char builtInCommands[8][10] = {"cd", "ls", "pwd", "gcc", "ps", "clear", "exit", "help"};

  bool isMatch = false;
  int i;

  for(i=0; i<8; ++i)
  {
    if(strcmp(command, builtInCommands[i]) == 0)
    {
      isMatch = true;

      break;
    }
  }

  return isMatch;
}

void execute(char* line)
{
  char* args[MAX_SIZE];
  char* argv[MAX_SIZE];
  char* path;

  size_t nargs = parse(line, args);

  int fd = -1;
  int flag = 0;

  int i, j;

  pid_t pid;
  int status;

  bool isBuiltIn = lookupBuildInCommand(args[0]);

  if(nargs == 0) return;

  /* REDIRECTION */
  for(i=0; i<(int)nargs; ++i)
  {
    if(strcmp(args[i], REDIRECT) == 0)
    {
      flag = O_WRONLY | O_CREAT | O_TRUNC;

      break;
    }
    else if(strcmp(args[i], APPEND) == 0)
    {
      flag = O_WRONLY | O_CREAT | O_APPEND;

      break;
    }
  }

  if(isBuiltIn == true)
  {
    path = (char*)malloc(strlen(args[0]) + 6);
    strcpy(path, "/bin/");
    strcpy(path, args[0]);

    pid = fork();

    if(strcmp(args[0], "exit") == 0)
    {
      exit(EXIT_SUCCESS);
    }
    else if(strcmp(args[0], "clear") == 0)
    {
      system("clear");
    }
    else if(strcmp(args[0], "help") == 0)
    {
      help();
    }
    else
    {
      pid = fork();

      switch(pid)
      {
        case -1:
        {
          perror("fork");

          return;
        }

        case 0:
        {
          for(j=0; j<i; ++j)
            argv[j] = args[j];

          if(flag > 0)
          {
            if((fd = open(args[i+1], flag, 0644)) == -1)
            {
              perror("open");

              return;
            }

            if(close(STDOUT_FILENOE) == -1)
            {
              perror("close");

              return;
            }

            if(dup2(fd, STDOUT_FILENO) == -1)
            {
              perror("dup2");

              return;
            }
          }

          if(execv(path, argv) == -1)
          {
            perror("execv");

            return;
          }

          exit(EXIT_SUCCESS);
        }

        default:
        {
          if(strcmp(args[nargs - 1], BACKGROUND) == 0)
            wait(&status);
        }
      }

      if(flag > 0)
      {
        if(close(fd) == -1)
        {
          perror("close");

          return;
        }
      }
    }
  }
  else
  {
    pid = fork();

    switch(pid)
    {
      case -1:
      {
        perror("fork");

        return;
      }

      case 0:
      {
        for(j=0; j<i; ++j)
          argv[j] = args[j];

        if(flag > 0)
        {
          if((fd = open(args[i+1], flag, 0644)) == -1)
          {
            perror("open");

            return;
          }

          if(close(STDOUT_FILENOE) == -1)
          {
            perror("close");

            return;
          }

          if(dup2(fd, STDOUT_FILENO) == -1)
          {
            perror("dup2");

            return;
          }
        }

        if(execvp(args[0], argv) == -1)
        {
          perror("execvp");

          return;
        }

        exit(EXIT_SUCCESS);
      }

      default:
      {
        if(strcmp(args[nargs - 1], BACKGROUND) == 0)
          wait(&status);
      }
    }

    if(flag > 0)
    {
      if(close(fd) == -1)
      {
        perror("close");

        return;
      }
    }
  }
}
