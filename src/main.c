/* main.c - IBM MQ diagnostic and error reporting tool.

   This is the main entry point for the diserr utility.
   It connects to an IBM MQ queue manager and performs diagnostic
   operations to report on errors and system status.

   Usage:
     diserr [OPTIONS]

   Options:
     -m, --qmgr      <name>  Queue manager name
     -l, --logfile   <name>  Error log file name (e.g. AMQERR01)
     -t, --timestamp <iso>   Show entries after ISO timestamp
     -h, --help              Display this help message
*/

#include <stdio.h>
#include <stdlib.h>
#include "cmdline.h"
#include "mqutil.h"

int main (int argc, char *argv[])
{
  mqconn_t conn;
  int rc;

  // -------------------------------------------------------
  // Initialize command line arguments.  
  // -------------------------------------------------------
  cmdlInit ();

  // -------------------------------------------------------
  // Parse command line arguments.  
  // -------------------------------------------------------
  rc = cmdlParse (argc, argv);
  switch (rc)
  {
    case 0:          // Success - continue.  
      break;         //
    case 1:          // Help requested - exit cleanly.  
      goto _door_ok; //
    case -1:         // Parse error - exit with failure.  
      goto _door_error;
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
    fprintf ( stderr, "Failed to connect to queue manager: %s\n",
              args.qmgr);
    goto _door_error;
  }

  // -------------------------------------------------------
  // Inquire error log.  
  // -------------------------------------------------------
  rc = mqInquireErrorLog (&conn);
  switch (rc)
  {
    case 0:           // Success - continue.  
      break;          //
    default:          // Error - exit with failure.  
      goto _door_error;
  }

  // -------------------------------------------------------
  // Disconnect from queue manager.  
  // -------------------------------------------------------
  if (mqDisconnect (&conn))
  {
    fprintf (stderr, "Failed to disconnect from queue manager: %s\n",
             args.qmgr);
    goto _door_error;
  }

  _door_ok:
    cmdlCleanup ();
    return EXIT_SUCCESS;

  _door_error:
    cmdlCleanup ();
    return EXIT_FAILURE;

}
