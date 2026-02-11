/* main.c - IBM MQ diagnostic and error reporting tool.

   This is the main entry point for the diserr utility.
   It connects to an IBM MQ queue manager and performs diagnostic
   operations to report on errors and system status.

   Usage: diserr -m <qmgr_name>
          diserr --qmgr <qmgr_name>
          diserr --help

   Options:
     -m, --qmgr <name>  Specify the queue manager name to connect to
     -h, --help         Display usage information
*/

#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h"
#include "mqutil.h"

int
main (int argc, char *argv[])
{
  cmdline_args_t args;
  mq_connection_t mq_conn;
  int ret;

  /* Initialize command line arguments.  */
  cmdline_init (&args);

  /* Parse command line arguments.  */
  ret = cmdline_parse (argc, argv, &args);
  if (ret != 0)
  {
      cmdline_cleanup (&args);
      return (ret < 0) ? EXIT_FAILURE : EXIT_SUCCESS;
  }

  /* Initialize MQ connection.  */
  mq_init (&mq_conn, args.qmgr_name);

  /* Connect to queue manager.  */
  if (mq_connect (&mq_conn) != 0)
  {
      fprintf (stderr, "Failed to connect to queue manager: %s\n",
               args.qmgr_name);
      cmdline_cleanup (&args);
      return EXIT_FAILURE;
  }

  /* TODO: Add your IBM MQ operations here.  */

  /* Disconnect from queue manager.  */
  mq_disconnect (&mq_conn);

  /* Cleanup.  */
  cmdline_cleanup (&args);

  return EXIT_SUCCESS;
}
