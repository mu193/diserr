/* main.c - IBM MQ diagnostic and error reporting tool.

   This is the main entry point for the diserr utility.
   It connects to an IBM MQ queue manager and performs diagnostic
   operations to report on errors and system status.

   Usage:
     diserr [OPTIONS]

   Options:
     -m, --qmgr        <name>  Connect in binding mode to this queue manager
     -c, --client      <name>  Connect in client mode to this queue manager
     -r, --remQmgr     <name>  Execute PCF on this remote queue manager
     -l, --logFile     <name>  Error log file name (e.g. AMQERR01)
     -t, --timeStamp   <iso>   Show entries after ISO 8601 timestamp
     -f, --format      <fmt>   Output format: text (default) or json
    -o, --output      <dest>  Output destination: stdout or a file name
     -v, --verbose             Print all PCF selectors (default: log records only)
     -h, --help                Display this help message
       --version             Print version information
*/

#define _POSIX_C_SOURCE 200809L  // fileno, fdopen, dup

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmdline.h"
#include "mqutil.h"
#include "version.h"

int main (int argc, char *argv[])
{
  mqconn_t conn;
  int rc;

  /**************************************************************************/
  /*                         I N I T I A L I Z A T I O N                    */
  /**************************************************************************/

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
    case 0:            // Success - continue.  
      break;           //
    case 1:            // Help requested - exit cleanly.  
      goto _door_ok;   //
    case 2:            // Version requested - exit cleanly.
      version ();
      goto _door_ok;
    case -1:           // Parse error - exit with failure.  
      goto _door_error;
  }

  // -------------------------------------------------------
  // Save the real stdout before possibly redirecting it to
  // a file with -o/--output, so --print-time can still write
  // timestamps to the terminal instead of the output file.
  // -------------------------------------------------------
  gRealStdout = fdopen (dup (fileno (stdout)), "w");
  if (gRealStdout == NULL)
  {
    fprintf (stderr, "Error: cannot duplicate stdout\n");
    goto _door_error;
  }

  if (strcmp (args.output, "stdout") != 0
      && freopen (args.output, "a", stdout) == NULL)
  {
    fprintf (stderr, "Error: cannot open output file '%s'\n", args.output);
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
              args.qmgr != NULL ? args.qmgr : "(default qmgr)");
    goto _door_error;
  }

  /**************************************************************************/
  /*                            R E A L   W O R K                           */
  /**************************************************************************/

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

  /**************************************************************************/
  /*                          H O U S E K E E P I N G                       */
  /**************************************************************************/

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
    if (gRealStdout != NULL) fclose (gRealStdout);
    cmdlCleanup ();
    return EXIT_SUCCESS;

  _door_error:
    if (gRealStdout != NULL) fclose (gRealStdout);
    cmdlCleanup ();
    return EXIT_FAILURE;

}
