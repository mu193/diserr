/* cmdline.c - Command line argument parsing.

   This module handles parsing and validation of command line arguments
   for the diserr utility. It uses getopt_long to support both short
   and long option formats.

   Functions:
     cmdlInit     - Initialize arguments structure with defaults
     cmdlParse    - Parse command line arguments from argc/argv
     cmdlUsage    - Display usage information to stdout
     cmdlCleanup  - Free allocated memory in arguments structure
*/

#define _POSIX_C_SOURCE 200809L // use POSIX.1-2008 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "cmdline.h"

cmdline_t args;

/******************************************************************************/
/* Initialize command line arguments structure with default values.           */
/******************************************************************************/
void cmdlInit (void)
{
  args.qmgr       = NULL;
  args.logFile    = NULL;
  args.timeStamp  = NULL;
  args.remQmgr     = NULL;
}

/******************************************************************************/
/* Display usage information.                                                 */
/******************************************************************************/
void cmdlUsage (const char *progname)
{
  printf ("Usage: %s [OPTIONS]\n", progname);
  printf ("\nOptions:\n");
  printf ("  -m, --qmgr      <name>   Queue manager name (default: default queue manager)\n");
  printf ("  -r, --remQmgr    <name>   Execute PCF on this remQmgr queue manager\n");
  printf ("  -l, --logFile   <name>   Error log file name (e.g. AMQERR01)\n");
  printf ("  -t, --timeStamp <iso>    Show entries after this ISO 8601 timeStamp\n");
  printf ("  -h, --help               Display this help message\n");
  printf ("\nTimestamp format (-t):\n");
  printf ("  Time components may be omitted from the right; missing parts default to 0.\n");
  printf ("  The timezone suffix controls how the time is interpreted:\n");
  printf ("\n");
  printf ("    No suffix     Local system time, automatically converted to UTC\n");
  printf ("                  e.g.  -t 2026-07-27T22:10\n");
  printf ("\n");
  printf ("    Z suffix      Explicit UTC (Zulu)\n");
  printf ("                  e.g.  -t 2026-07-27T22:10:11.226Z\n");
  printf ("\n");
  printf ("    +HH:MM        Explicit UTC east offset (e.g. CET=+01:00, CEST=+02:00)\n");
  printf ("                  e.g.  -t 2026-07-27T22:10+02:00\n");
  printf ("\n");
  printf ("    -HH:MM        Explicit UTC west offset (e.g. EST=-05:00)\n");
  printf ("                  e.g.  -t 2026-07-27T22:10-05:00\n");
  printf ("\n");
  printf ("  In all cases the timeStamp is normalised to yyyy-mm-ddThh:mm:ss.sssZ\n");
  printf ("  before being sent to the queue manager command server.\n");
}

/******************************************************************************/
/* Parse command line arguments.                                              */
/******************************************************************************/
int cmdlParse (int argc, char *argv[])
{
  int opt;
  int option_index = 0;

  static struct option long_options[] = {
    {"qmgr",      required_argument, 0, 'm'},
    {"remQmgr",    required_argument, 0, 'r'},
    {"logFile",   required_argument, 0, 'l'},
    {"timeStamp", required_argument, 0, 't'},
    {"help",      no_argument,       0, 'h'},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long (argc, argv, "m:r:l:t:h",
                             long_options, &option_index)) != -1)
  {
    switch (opt)
    {
      case 'm':
        args.qmgr = strdup (optarg);
        if (args.qmgr == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      case 'r':
        args.remQmgr = strdup (optarg);
        if (args.remQmgr == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      case 'l':
        args.logFile = strdup (optarg);
        if (args.logFile == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      case 't':
        args.timeStamp = strdup (optarg);
        if (args.timeStamp == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      case '?':
      case 'h':
        cmdlUsage (argv[0]);
        goto _door_ok;

      default:
        goto _door_error;
    }
  }

  _door_ok:
    return 0;

  _door_error:
    return 1 ;
}

/******************************************************************************/
/* Free allocated memory in command line arguments structure.                 */
/******************************************************************************/
void cmdlCleanup (void)
{
  if (args.qmgr != NULL)
  {
    free (args.qmgr);
    args.qmgr = NULL;
  }

  if (args.logFile != NULL)
  {
    free (args.logFile);
    args.logFile = NULL;
  }

  if (args.timeStamp != NULL)
  {
    free (args.timeStamp);
    args.timeStamp = NULL;
  }

  if (args.remQmgr != NULL)
  {
    free (args.remQmgr);
    args.remQmgr = NULL;
  }
}
