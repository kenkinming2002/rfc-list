#ifndef SPAWN_H
#define SPAWN_H

#include <unistd.h>
#include <stdio.h>

struct child
{
  pid_t pid;

  FILE *stdin;
  FILE *stdout;
};

#define SPAWN_FLAG_REDIRECT_STDIN (1 << 0)
#define SPAWN_FLAG_REDIRECT_STDOUT (1 << 1)

int spawnvp(struct child *child, int flags, const char *file, char *const argv[]);

#endif // SPAWN_H
