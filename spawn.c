#include "spawn.h"

#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

// Some system calls should practically never ever fail on any modern/reasonable
// operating system, but we technically should still check for error. In such a
// case, we simply exit the program for much simpler resource management.
#define CHECKED_NEGATIVE_ONE(expr) ({ typeof((expr)) result = (expr); if(result == -1) { perror(#expr); exit(EXIT_FAILURE); } result; })
#define CHECKED_NULL(expr) ({ typeof((expr)) result = (expr); if(result == NULL) { perror(#expr); exit(EXIT_FAILURE); } result; })

int spawnvp(struct child *child, int flags, const char *file, char *const argv[])
{
  int pipes_errno[2];
  CHECKED_NEGATIVE_ONE(pipe(pipes_errno));

  int pipes_stdin[2];
  if(flags & SPAWN_FLAG_REDIRECT_STDIN)
    CHECKED_NEGATIVE_ONE(pipe(pipes_stdin));

  int pipes_stdout[2];
  if(flags & SPAWN_FLAG_REDIRECT_STDOUT)
    CHECKED_NEGATIVE_ONE(pipe(pipes_stdout));

  switch((child->pid = fork()))
  {
  case 0:
    CHECKED_NEGATIVE_ONE(close(pipes_errno[0]));
    CHECKED_NEGATIVE_ONE(fcntl(pipes_errno[1], F_SETFD, CHECKED_NEGATIVE_ONE(fcntl(pipes_errno[1], F_GETFD)) | FD_CLOEXEC));

    if(flags & SPAWN_FLAG_REDIRECT_STDIN)
    {
      CHECKED_NEGATIVE_ONE(dup2(pipes_stdin[0], STDIN_FILENO));
      CHECKED_NEGATIVE_ONE(close(pipes_stdin[0]));
      CHECKED_NEGATIVE_ONE(close(pipes_stdin[1]));
    }

    if(flags & SPAWN_FLAG_REDIRECT_STDOUT)
    {
      CHECKED_NEGATIVE_ONE(dup2(pipes_stdout[1], STDOUT_FILENO));
      CHECKED_NEGATIVE_ONE(close(pipes_stdout[0]));
      CHECKED_NEGATIVE_ONE(close(pipes_stdout[1]));
    }

    if(execvp(file, argv) != 0)
    {
      write(pipes_errno[1], &errno, sizeof errno);
      exit(EXIT_FAILURE);
    }

  default:
    CHECKED_NEGATIVE_ONE(close(pipes_errno[1]));

    if(read(pipes_errno[0], &errno, sizeof errno) == sizeof errno)
    {
      CHECKED_NEGATIVE_ONE(close(pipes_errno[0]));

      if(flags & SPAWN_FLAG_REDIRECT_STDIN)
      {
        CHECKED_NEGATIVE_ONE(close(pipes_stdin[0]));
        CHECKED_NEGATIVE_ONE(close(pipes_stdin[1]));
      }

      if(flags & SPAWN_FLAG_REDIRECT_STDOUT)
      {
        CHECKED_NEGATIVE_ONE(close(pipes_stdout[0]));
        CHECKED_NEGATIVE_ONE(close(pipes_stdout[1]));
      }

      return -1;
    }

    CHECKED_NEGATIVE_ONE(close(pipes_errno[0]));

    if(flags & SPAWN_FLAG_REDIRECT_STDIN)
    {
      CHECKED_NEGATIVE_ONE(close(pipes_stdin[0]));
      child->stdin = CHECKED_NULL(fdopen(pipes_stdin[1], "w"));
    }
    else
      child->stdin = NULL;

    if(flags & SPAWN_FLAG_REDIRECT_STDOUT)
    {
      CHECKED_NEGATIVE_ONE(close(pipes_stdout[1]));
      child->stdout = CHECKED_NULL(fdopen(pipes_stdout[0], "r"));
    }
    else
      child->stdout = NULL;

    return 0;

  case -1:
    return -1;
  }
}

