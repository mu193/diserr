/* main.c - IBM MQ diagnostic and error reporting tool.

   This is the main entry point for the diserr utility.
   It connects to an IBM MQ queue manager and performs diagnostic
   operations to report on errors and system status.

   Usage:
     diserr -m <qmgr_name>
     diserr --qmgr <qmgr_name>
     diserr --help

   Options:
     -m, --qmgr <name>  Queue manager name to connect to
     -h, --help         Display usage information
*/

#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h"
#include "mqutil.h"

int main (int argc, char *argv[])
{
  cmdline_t args;
  mqconn_t mq_conn;
  int ret;

  /* Initialize command line arguments.  */
  cmdlInit (&args);

  /* Parse command line arguments.  */
  ret = cmdlParse (argc, argv, &args);
  if (ret != 0)
  {
      cmdlCleanup (&args);
      return (ret < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
  }

  /* Initialize MQ connection.  */
  mqInit (&mq_conn, args.qmgr_name);

  /* Connect to queue manager.  */
  if (mqConnect (&mq_conn) != 0)
  {
      fprintf (stderr, "Failed to connect to queue manager: %s\n",
               args.qmgr_name);
      cmdlCleanup (&args);
      return EXIT_FAILURE;
  }

  /* TODO: Add your IBM MQ operations here.  */

  /* Disconnect from queue manager.  */
  mqDisconnect (&mq_conn);

  /* Cleanup.  */
  cmdlCleanup (&args);

  return EXIT_SUCCESS;
}
