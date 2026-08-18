/* osutil.c - OS-level helper implementation.

   This module provides helpers for process and pipe orchestration.

   Functions:
     amq2json - Run mqrc and convert text input to JSON output
*/

#define _POSIX_C_SOURCE 200809L  // PIPE_BUF, fork, pipe, waitpid

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include <errno.h>
#include <limits.h>
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
int amq2json( const char  *text ,  // input: text to convert (may be NULL)
                    char **json )  // output: pointer to heap buffer with JSON
{
  int inPipe[2];     // pipe for writing text to mqrc's stdin
  int outPipe[2];    // pipe for reading JSON from mqrc's stdout
  long maxFd;        // maximum file descriptor number for closing in child
  int status;

  if (text == NULL || json == NULL) // handle wrong arguments
  {
    errno = EINVAL;
    return -1;
  }
  *json = NULL;

  if (pipe (inPipe) != 0)   // open pipe failes
    return -1;              // if process file descriptor limit (RLIMIT_NOFILE) 
  if (pipe (outPipe) != 0)  //     is too low
  {                         // if system wide files descriptor limit is reached
    close (inPipe[0]);      // if the kernel is out of resources 
    close (inPipe[1]);      //     and cannot create a new pipe
    return -1;
  }

  /**************************************************************************/
  /*                                F O R K                                 */
  /**************************************************************************/
  pid_t pidMqrc = fork ();    
                       
  // -------------------------------------------------------
  // fork failed
  // -------------------------------------------------------
  if (pidMqrc < 0)           
  {                      
    close (inPipe[0]);   
    close (inPipe[1]);   
    close (outPipe[0]);  
    close (outPipe[1]); 
    return -1;
  }

  // -------------------------------------------------------
  // child process: execute mqrc
  // -------------------------------------------------------
  if (pidMqrc == 0)          
  {
    dup2 (inPipe[0], STDIN_FILENO);
    dup2 (outPipe[1], STDOUT_FILENO);

    close (inPipe[0]);
    close (inPipe[1]);
    close (outPipe[0]);
    close (outPipe[1]);

    // -----------------------------------------------------
    // Close all file descriptors except stdin/stdout/stderr 
    // inherited from the parent process to avoid leaking them 
    // into the child process. 
    // -----------------------------------------------------
    maxFd = sysconf (_SC_OPEN_MAX); // get maximum number of file descriptors
    if (maxFd < 0)                  // if sysconf fails, 
      maxFd = 1024;                 // |--> use a reasonable default
    for (int fd = 3; fd < maxFd; ++fd)
      close (fd);
    
  // -------------------------------------------------------
  // Execute mqrc with arguments to convert text to JSON.
  //   exec : execute
  //   l    : attributes as a list
  //   p    : search PATH for executable
  // -------------------------------------------------------
    execlp ("mqrc", "mqrc", "-i", "text", "-o", "json", "-", (char *) NULL);
    _exit (127);
  } 

  // -------------------------------------------------------
  // parent process: close unused pipe ends and write text to child
  // -------------------------------------------------------
  close (inPipe[0]);
  close (outPipe[1]);

  // -------------------------------------------------------
  // Fork a writer child to avoid deadlock: 
  //   if mqrc's output fills its pipe buffer it blocks, 
  //   which stops it reading stdin, which blocks the write.
  // The solution is to fork a second child to write the 
  //   input text to mqrc's stdin. 
  // -------------------------------------------------------
  pid_t pidWriter = fork ();  // fork a writer child
  if (pidWriter < 0)          //
  {                      // fork failed
    close (inPipe[1]);   // close write end of input pipe
    close (outPipe[0]);
    waitpid (pidMqrc, NULL, 0);
    return -1;
  }
  if (pidWriter == 0)         // writer child
  {                      // write text to mqrc's stdin and exit
    close (outPipe[0]);  // close read end of output pipe
    // -----------------------------------------------------
    // Write text to mqrc's stdin. Loop until all bytes are written.
    // write may return a short count if len > PIPE_BUF; 
    //   PIPE_BUF is 4096 on Linux, 512 on AIX, 1024 on Windows, 2048 on z/OS;
    //   mqrc will produce more than that in one write
    // -----------------------------------------------------

    size_t len = strlen( text );
    size_t off = 0;     
    while (off < len)    // loop resumes from where it left off
    {
      ssize_t nw = write (inPipe[1], text + off, len - off);
      if (nw < 0)
      {
        if (errno == EINTR) continue;
        break;
      }
      off += (size_t) nw;
    }
    close (inPipe[1]);
    _exit (0);
  }
  close (inPipe[1]);

  // -------------------------------------------------------
  // real parent process: 
  //     read JSON output from mqrc's stdout into a heap buffer.
  // -------------------------------------------------------
  char  *buf  = NULL;  // heap buffer for accumulating JSON output
  size_t used = 0;     // number of bytes used in buf
  size_t cap  = 0;     // capacity of buf
  while (1)
  {
    char tmp[PIPE_BUF];
    ssize_t n = read (outPipe[0], tmp, sizeof (tmp)); 
    if (n == 0)   // nothing more to read (EOF)
      break;      //
    if (n < 0)    // read error
    {             //
      if (errno == EINTR)
        continue;
      close (outPipe[0]);
      waitpid (pidMqrc, NULL, 0);
      free (buf);
      return -1;
    }

    if (used + (size_t) n + 1 > cap)
    {
      size_t newCap = (cap == 0) ? 8192 : cap * 2; // alloc at least 8k,
      while (newCap < used + (size_t) n + 1)       // double each time
        newCap *= 2;                               // and realloc until it fits
      char *newBuf = realloc (buf, newCap);
      if (newBuf == NULL)
      {
        close (outPipe[0]);
        waitpid (pidMqrc, NULL, 0);
        free (buf);
        errno = ENOMEM;
        return -1;
      }
      buf = newBuf;
      cap = newCap;
    }
    memcpy (buf + used, tmp, (size_t) n);
    used += (size_t) n;
  }                     // whole stream has been read, 
  close (outPipe[0]);   // close pipe to mqrc's stdout

  waitpid (pidWriter, NULL, 0);
  if (waitpid (pidMqrc, &status, 0) < 0)
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
