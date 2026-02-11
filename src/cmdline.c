/* cmdline.c - Command line argument parsing.

   This module handles parsing and validation of command line arguments
   for the diserr utility. It uses getopt_long to support both short
   and long option formats.

   Functions:
     cmdline_init     - Initialize arguments structure with defaults
     cmdline_parse    - Parse command line arguments from argc/argv
     cmdline_usage    - Display usage information to stdout
     cmdline_cleanup  - Free allocated memory in arguments structure
*/

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include "cmdline.h"

/* Initialize command line arguments structure with default values.  */
void
cmdline_init (cmdline_args_t *args)
{
  args->qmgr_name = NULL;
}

/* Display usage information.  */
void
cmdline_usage (const char *progname)
{
  printf ("Usage: %s [OPTIONS]\n", progname);
  printf ("\nOptions:\n");
  printf ("  -m, --qmgr <name>    Queue manager name\n");
  printf ("  -h, --help           Display this help message\n");
}

/* Parse command line arguments.  */
int
cmdline_parse (int argc, char *argv[], cmdline_args_t *args)
{
  int opt;
  int option_index = 0;

  static struct option long_options[] = {
    {"qmgr", required_argument, 0, 'm'},
    {"help", no_argument,       0, 'h'},
    {0, 0, 0, 0}
  };

  while ((opt = getopt_long (argc, argv, "m:h",
                             long_options, &option_index)) != -1)
  {
      switch (opt)
      {
      case 'm':
          args->qmgr_name = strdup (optarg);
          if (args->qmgr_name == NULL)
          {
              fprintf (stderr, "Error: Memory allocation failed\n");
              return -1;
          }
          break;

      case 'h':
          cmdline_usage (argv[0]);
          return 1;

      case '?':
          /* getopt_long already printed an error message.  */
          cmdline_usage (argv[0]);
          return -1;

      default:
          return -1;
      }
  }

  /* Check for required arguments.  */
  if (args->qmgr_name == NULL)
  {
      fprintf (stderr, "Error: Queue manager name (-m/--qmgr) is required\n");
      cmdline_usage (argv[0]);
      return -1;
  }

  return 0;
}

/* Free allocated memory in command line arguments structure.  */
void
cmdline_cleanup (cmdline_args_t *args)
{
  if (args->qmgr_name != NULL)
  {
      free (args->qmgr_name);
      args->qmgr_name = NULL;
  }
}
