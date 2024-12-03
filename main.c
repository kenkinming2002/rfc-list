#include "spawn.h"

#include <curl/curl.h>

#include <sys/wait.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <errno.h>

static int fetch(const char *url, FILE *file)
{
  CURL *curl = curl_easy_init();
  if(!curl)
    return -1;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
  if(curl_easy_perform(curl) != CURLE_OK)
    return -1;

  curl_easy_cleanup(curl);
  return 0;
}

static char *fetch_text(const char *url)
{
  char *data;
  size_t length;

  FILE *file = open_memstream(&data, &length);
  if(!file)
    return NULL;

  if(fetch(url, file) != 0)
    return NULL;

  fclose(file);
  return data;
}

static char *strtok2(char *restrict str, const char *restrict delim)
{
  static char *saveptr;
  if(str)
    saveptr = str;

  if(!*saveptr)
    return NULL;

  size_t count;
  char *p;
  for(count = 0, p = saveptr; *p; ++p)
    if(strchr(delim, *p))
      ++count;
    else if(count < 2)
      count = 0;
    else
      break;

  char *result = saveptr;
  *(p - count) = '\0';
  saveptr = p;
  return result;
}

static char *strdel(char *restrict str, const char *restrict reject)
{
  char *dst = str;
  for(char *p = str; *p; ++p)
    if(!strchr(reject, *p))
      *dst++ = *p;

  *dst++ = '\0';
  return str;
}

static char *strdel2(char *restrict str, const char *restrict reject)
{
  char *dst = str;

  size_t count = 0;
  for(char *p = str; *p; ++p)
  {
    if(strchr(reject, *p))
      ++count;
    else
      count = 0;

    if(count < 2)
      *dst++ = *p;
  }

  *dst++ = '\0';
  return str;
}

static bool is_rfc_entry(const char *str)
{
  if(strlen(str) < 5)
    return false;

  for(size_t i=0; i<4; ++i)
    if(str[i] < '0' || str[i] > '9' )
      return false;

  if(str[4] != ' ')
    return false;

  return true;
}

static int spawn_fzf(struct child *child, int flags)
{
  static char *fzf_argv[] = {"fzf", NULL};
  return spawnvp(child, flags, "fzf", fzf_argv);
}

static int spawn_pager_impl(struct child *child, int flags, char *name)
{
  char *argv[] = {name, NULL};
  return spawnvp(child, flags, name, argv);
}

static int spawn_pager(struct child *child, int flags)
{
  char *pager;
  if((pager = getenv("PAGER")) && spawn_pager_impl(child, flags, pager) == 0)
    return 0;

  if(spawn_pager_impl(child, flags, "less") == 0) return 0;
  if(spawn_pager_impl(child, flags, "more") == 0) return 0;

  return -1;
}

static int fzf_input(const struct child fzf, char *rfc_index)
{
  for(char *line = strtok2(rfc_index, "\n"); line; line = strtok2(NULL, "\n"))
  {
    line = strdel(line, "\n");
    line = strdel2(line, " ");
    if(is_rfc_entry(line))
      if(fprintf(fzf.stdin, "%s\n", line) < 0)
        return -1;
  }

  if(fflush(fzf.stdin) != 0)
    return -1;

  return 0;
}

static int fzf_output(const struct child fzf, unsigned *id)
{
  char buf[4];
  if(fread(buf, sizeof buf, 1, fzf.stdout) != 1)
    return -1;

  *id = 0;
  for(unsigned i=0; i<sizeof buf; ++i)
  {
    if(buf[i] < '0' || buf[i] > '9')
      return -1;

    *id *= 10;
    *id += buf[i] - '0';
  }
  return 0;
}

int main()
{
  struct child fzf;
  if(spawn_fzf(&fzf, SPAWN_FLAG_REDIRECT_STDIN | SPAWN_FLAG_REDIRECT_STDOUT) == -1)
  {
    fprintf(stderr, "Error: Failed to spawn fzf. Do you have it installed?\n");
    return -1;
  }

  char *rfc_index_text;
  if(!(rfc_index_text = fetch_text("https://www.rfc-editor.org/rfc/rfc-index.txt")))
  {
    fprintf(stderr, "Error: Failed to fetch rfc index.\n");
    return -1;
  }

  if(fzf_input(fzf, rfc_index_text) != 0)
  {
    fprintf(stderr, "Error: Failed to provide data to fzf\n");
    return -1;
  }

  unsigned id;
  if(fzf_output(fzf, &id) != 0)
  {
    fprintf(stderr, "Error: Failed to read response from fzf\n");
    return -1;
  }

  if(waitpid(fzf.pid, NULL, 0) == -1)
  {
    fprintf(stderr, "Error: Failed to wait for fzf to exit: %s\n", strerror(errno));
    return -1;
  }

  char *rfc_url;
  asprintf(&rfc_url, "https://www.rfc-editor.org/rfc/rfc%u.txt", id);

  struct child pager;
  if(spawn_pager(&pager, SPAWN_FLAG_REDIRECT_STDIN) == -1)
  {
    fprintf(stderr, "Error: Failed to spawn pager.\n");
    return -1;
  }

  if(fetch(rfc_url, pager.stdin) != 0)
  {
    fprintf(stderr, "Error: Failed to fetch rfc.\n");
    return -1;
  }

  if(fclose(pager.stdin) == -1)
  {
    fprintf(stderr, "Error: Failed to flush stdin of pager.\n");
    return -1;
  }

  if(waitpid(pager.pid, NULL, 0) == -1)
  {
    fprintf(stderr, "Error: Failed to wait for pager to exit: %s\n", strerror(errno));
    return -1;
  }

}
