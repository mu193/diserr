/* osutil.c - OS-level helper implementation.

   This module provides helpers for process and pipe orchestration.

   Functions:
     osutilMqrcTextToJson - Run mqrc and convert text input to JSON output
*/

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "osutil.h"

/******************************************************************************/
/*                                                                            */
/*                             F U N C T I O N S                              */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/* Run mqrc, write text into stdin, read JSON from stdout into heap buffer.   */
/******************************************************************************/
int osutilMqrcTextToJson (const char *text, char **json)
{
  int inPipe[2];
  int outPipe[2];
  pid_t pid;
  char *buf = NULL;
  size_t used = 0;
  size_t cap = 0;
  long maxFd;
  ssize_t n;
  int status;

  if (text == NULL || json == NULL)
  {
    errno = EINVAL;
    return -1;
  }
  *json = NULL;

  if (pipe (inPipe) != 0)
    return -1;
  if (pipe (outPipe) != 0)
  {
    close (inPipe[0]);
    close (inPipe[1]);
    return -1;
  }

  pid = fork ();
  if (pid < 0)
  {
    close (inPipe[0]);
    close (inPipe[1]);
    close (outPipe[0]);
    close (outPipe[1]);
    return -1;
  }

  if (pid == 0)
  {
    dup2 (inPipe[0], STDIN_FILENO);
    dup2 (outPipe[1], STDOUT_FILENO);

    close (inPipe[0]);
    close (inPipe[1]);
    close (outPipe[0]);
    close (outPipe[1]);

    maxFd = sysconf (_SC_OPEN_MAX);
    if (maxFd < 0)
      maxFd = 1024;
    for (int fd = 3; fd < maxFd; ++fd)
      close (fd);

    execlp ("mqrc", "mqrc", "-i", "text", "-o", "json", "-", (char *) NULL);
    _exit (127);
  }

  close (inPipe[0]);
  close (outPipe[1]);

  {
    size_t len = strlen (text);
    size_t off = 0;
    while (off < len)
    {
      n = write (inPipe[1], text + off, len - off);
      if (n < 0)
      {
        if (errno == EINTR)
          continue;
        close (inPipe[1]);
        close (outPipe[0]);
        waitpid (pid, NULL, 0);
        free (buf);
        return -1;
      }
      off += (size_t) n;
    }
  }
  close (inPipe[1]);

  while (1)
  {
    char tmp[4096];
    n = read (outPipe[0], tmp, sizeof (tmp));
    if (n == 0)
      break;
    if (n < 0)
    {
      if (errno == EINTR)
        continue;
      close (outPipe[0]);
      waitpid (pid, NULL, 0);
      free (buf);
      return -1;
    }

    if (used + (size_t) n + 1 > cap)
    {
      size_t newCap = (cap == 0) ? 8192 : cap * 2;
      while (newCap < used + (size_t) n + 1)
        newCap *= 2;
      char *newBuf = realloc (buf, newCap);
      if (newBuf == NULL)
      {
        close (outPipe[0]);
        waitpid (pid, NULL, 0);
        free (buf);
        errno = ENOMEM;
        return -1;
      }
      buf = newBuf;
      cap = newCap;
    }
    memcpy (buf + used, tmp, (size_t) n);
    used += (size_t) n;
  }
  close (outPipe[0]);

  if (waitpid (pid, &status, 0) < 0)
  {
    free (buf);
    return -1;
  }
  if (!WIFEXITED (status) || WEXITSTATUS (status) != 0)
  {
    free (buf);
    errno = EPROTO;
    return -1;
  }

  if (buf == NULL)
  {
    buf = malloc (1);
    if (buf == NULL)
      return -1;
  }
  buf[used] = '\0';
  *json = buf;
  return 0;
}
