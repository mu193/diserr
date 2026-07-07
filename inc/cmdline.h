/* cmdline.h - Command line argument parsing interface.

   This header defines the argument structure and function prototypes
   for the cmdline module.

   Types:
     cmdline_t - Structure holding parsed command line arguments

   Functions:
     cmdlInit     - Initialize arguments structure with defaults
     cmdlParse    - Parse command line arguments from argc/argv
     cmdlUsage    - Display usage information to stdout
     cmdlCleanup  - Free allocated memory in arguments structure
*/

#ifndef CMDLINE_H
#define CMDLINE_H

/* Parsed command line arguments.  */
typedef struct
{
  char *qmgr;
} cmdline_t;

void cmdlInit    (cmdline_t *args);
void cmdlUsage   (const char *progname);
int  cmdlParse   (int argc, char *argv[], cmdline_t *args);
void cmdlCleanup (cmdline_t *args);

#endif /* CMDLINE_H */
