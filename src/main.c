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
      -d, --daemon              Collect data from all configured remote queue managers
       --add-qmgr    <name>   Add queue manager to daemon configuration
     -v, --verbose             Print all PCF selectors (default: log records only)
         --print-time          Print timestamp with each log entry
     -h, --help                Display this help message
         --version             Print version information
*/

/******************************************************************************/
/*                                D E F I N E S                               */
/******************************************************************************/

// -------------------------------------
// compiler features 
// -------------------------------------
#define _POSIX_C_SOURCE 200809L  // fileno, fdopen, dup

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmdline.h"
#include "mqlogctl.h"
#include "mqutil.h"
#include "version.h"

int foreground (mqconn_t *conn);
int daemonize  (mqconn_t *conn);

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
  // Handle adding a new queue manager if requested.
  // -------------------------------------------------------
  if (args.nodeQmgr != NULL)
  {
    if (!args.config)
    {
      fprintf (stderr, "Error: --add-qmgr requires --config\n");
      goto _door_error;
    }
    rc = addQmgr (&conn);
    if (rc)
      goto _door_error;
    goto _door_ok;
  }

  // -------------------------------------------------------
  // Run as a daemon or in the foreground.
  // -------------------------------------------------------
  if (args.daemon)
  {
    rc = daemonize (&conn);
  }
  else
  {
    rc = foreground (&conn);
  }
  if (rc)
    goto _door_error;

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

/******************************************************************************/
/* Run in foreground mode.                                                    */
/******************************************************************************/
int foreground (mqconn_t *conn)
{
  int rc;

  // -------------------------------------------------------
  // Save the real stdout before possibly redirecting it to
  // a file with -o/--output, so --print-time can still write
  // timestamps to the terminal instead of the output file.
  // -------------------------------------------------------
  gRealStdout = fdopen (dup (fileno (stdout)), "w");
  if (gRealStdout == NULL)
  {
    fprintf (stderr, "Error: cannot duplicate stdout\n");
    rc = -1;
    goto _door;
  }

  if (strcmp (args.output, "stdout") != 0
      && freopen (args.output, "a", stdout) == NULL)
  {
    fprintf (stderr, "Error: cannot open output file '%s'\n", args.output);
    rc = -1;
    goto _door;
  }

  // -------------------------------------------------------
  // Inquire error log.
  // -------------------------------------------------------
  rc = mqInquireErrorLog (conn);
  switch (rc)
  {
    case 0:           // Success - continue.
      break;          //
    default:          // Error - exit with failure.
      rc = -1;
      goto _door;
  }

  _door:

  return rc;
}

/******************************************************************************/
/* Run in daemon mode.                                                        */
/******************************************************************************/
int daemonize (mqconn_t *conn)
{
  MQHOBJ hCtrlQ = MQHO_NONE;
  int mqrc;
  int rc = 0;

  // -------------------------------------------------------
  // open control queue.
  // -------------------------------------------------------
  mqrc = mqOpenQueue( conn              ,
                      MQ_DISERR_CTRL_QUEUE,
                      NULL              ,
                      MQOO_INPUT_SHARED | MQOO_OUTPUT          |\
                      MQOO_BROWSE       | MQOO_FAIL_IF_QUIESCING,
                      &hCtrlQ          );

  switch( mqrc )
  {
    case MQRC_NONE:   // Success - continue.
      break;          //
    default:          // Error - exit with failure.
      rc = -1;
      goto _door;
  }

  // -------------------------------------------------------
  // Close control queue.
  // -------------------------------------------------------
  mqrc = mqCloseQueue (conn, &hCtrlQ);
  switch (mqrc)
  {
    case MQRC_NONE:   // Success - continue.
      break;          //
    default:          // Error - exit with failure.
      rc = -1;
      goto _door;
  }

  _door:

  return rc;
}
