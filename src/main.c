/* main.c - IBM MQ diagnostic and error reporting tool.

   This is the main entry point for the diserr utility.
   It connects to an IBM MQ queue manager and performs diagnostic
   operations to report on errors and system status.

   Usage:
     diserr -m <qmgr>
     diserr --qmgr <qmgr>
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
  mqconn_t conn;
  int ret;

  // -------------------------------------------------------
  // Initialize command line arguments.  
  // -------------------------------------------------------
  cmdlInit ();

  // -------------------------------------------------------
  // Parse command line arguments.  
  // -------------------------------------------------------
  ret = cmdlParse (argc, argv);
  switch (ret)
  {
  case 0:      // Success - continue.  
      break;   //
  case 1:      // Help requested - exit cleanly.  
      cmdlCleanup ();
      return EXIT_SUCCESS;
  case -1:     // Parse error - exit with failure.  
      cmdlCleanup ();
      return EXIT_FAILURE;
  }

  // -------------------------------------------------------
  // Initialize MQ connection.  
  // -------------------------------------------------------
  mqInit (&conn);

  // -------------------------------------------------------
  // Connect to queue manager.  
  // -------------------------------------------------------
  if (mqConnect (&conn))
  {
      fprintf (stderr, "Failed to connect to queue manager: %s\n",
               args.qmgr);
      cmdlCleanup ();
      return EXIT_FAILURE;
  }

  // -------------------------------------------------------
  // Inquire error log.  
  // -------------------------------------------------------
  mqInquireErrorLog (&conn);

  // -------------------------------------------------------
  // Disconnect from queue manager.  
  // -------------------------------------------------------
  if (mqDisconnect (&conn))
  {
      cmdlCleanup ();
      return EXIT_FAILURE;
  }

  // -------------------------------------------------------
  // Cleanup.  
  // -------------------------------------------------------
  cmdlCleanup ();

  return EXIT_SUCCESS;
}
