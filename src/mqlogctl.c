/* mqlogctl.c - IBM MQ error log control functions.

   This module will contain functions for controlling and retrieving
   queue-manager error log files.

   Functions:
     addQmgr    - register a queue manager in the daemon configuration
     listQmgr   - list queue-manager entries from daemon configuration
     foreground - run in foreground mode
     daemonize  - run in daemon mode
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
#include "mqlogctl.h"
#include "cmdline.h"
#include "mqreason.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>

/******************************************************************************/
/*                                                                            */
/*                             F U N C T I O N S                              */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/* Add queue manager to daemon configuration.                                 */
/******************************************************************************/
int addQmgr (mqconn_t *conn)
{
  MQHOBJ hCtrlQ = MQHO_NONE;
  MQLONG mqcc, mqrc;
  int rc = 0;

  // -------------------------------------------------------
  // open control queue for browse and put.
  // -------------------------------------------------------
  mqrc = mqOpenQueue( conn                ,
                      MQ_DISERR_CTRL_QUEUE,
                      NULL                ,
                      MQOO_BROWSE | MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING,
                      &hCtrlQ);
  switch (mqrc)
  {
    case MQRC_NONE:
      break;
    default:
      rc = -1;
      goto _door;
  }

  // -------------------------------------------------------
  // build fixed-width, space-padded qmgr name to add.
  // -------------------------------------------------------
  char newQmgr[MQ_Q_MGR_NAME_LENGTH];
  size_t nodeQmgrLen = strlen (args.nodeQmgr);

  memset (newQmgr, ' ', sizeof (newQmgr));
  memcpy (newQmgr, args.nodeQmgr, nodeQmgrLen);

  // -------------------------------------------------------
  // reject if a control message for this qmgr already exists.
  // -------------------------------------------------------
  MQGMO gmo = {MQGMO_DEFAULT};
  gmo.Options = MQGMO_BROWSE_FIRST | MQGMO_FAIL_IF_QUIESCING;

  for (;;)
  {
    // reset MsgId/CorrelId each call, else BROWSE_NEXT tries to match the previous message.
    MQMD md = {MQMD_DEFAULT};
    mqlogctl_t ctrlMsg = MQLOGCTL_DEFAULT;
    MQLONG dataLen = 0;

    MQGET( conn->hConn, hCtrlQ, &md, &gmo,
           sizeof (ctrlMsg), &ctrlMsg, &dataLen, &mqcc, &mqrc );

    if( mqrc == MQRC_NO_MSG_AVAILABLE )
      break;

    if (mqrc != MQRC_NONE)
    {
      fprintf (stderr, "MQGET browse failed: RC=%d %s\n", (int) mqrc, 
                                                          mqrc2str (mqrc));
      rc = -1;
      goto _door;
    }

    if (memcmp (ctrlMsg.qmgr, newQmgr, sizeof (newQmgr)) == 0)
    {
      fprintf (stderr, "qmgr '%s' is already registered.\n", args.nodeQmgr);
      rc = -1;
      goto _door;
    }

    gmo.Options = MQGMO_BROWSE_NEXT | MQGMO_FAIL_IF_QUIESCING;
  }

  // -------------------------------------------------------
  // put initial control message to the queue.
  // -------------------------------------------------------
  mqlogctl_t ctrlMsg = MQLOGCTL_DEFAULT;

  memcpy (ctrlMsg.qmgr, newQmgr, sizeof (newQmgr));

  mqrc = mqPutMsg (conn, hCtrlQ, &ctrlMsg, sizeof (ctrlMsg));

  switch (mqrc)
  {
    case MQRC_NONE:
      break;
    default:
      rc = -1;
      goto _door;
  }

  _door:
  if (hCtrlQ != MQHO_NONE && mqCloseQueue (conn, &hCtrlQ) != MQRC_NONE)
    rc = -1;

  return rc;
}

/******************************************************************************/
/* List queue managers from daemon configuration.                             */
/******************************************************************************/
int listQmgr (mqconn_t *conn)
{
  MQHOBJ hCtrlQ = MQHO_NONE;
  MQLONG mqcc, mqrc;
  int sysRc = 0;

  // -------------------------------------------------------
  // Open control queue for browse.
  // -------------------------------------------------------
  mqrc = mqOpenQueue( conn                ,
                      MQ_DISERR_CTRL_QUEUE,
                      NULL                ,
                      MQOO_BROWSE | MQOO_INQUIRE | MQOO_FAIL_IF_QUIESCING,
                      &hCtrlQ    );
  if (mqrc != MQRC_NONE)
  {
    fprintf (stderr, "mqOpenQueue browse failed: RC=%d %s\n", (int) mqrc, mqrc2str (mqrc));
    sysRc = -1;
    goto _door;
  }

  MQGMO gmo = {MQGMO_DEFAULT};
  gmo.Options = MQGMO_BROWSE_FIRST | MQGMO_FAIL_IF_QUIESCING;

  for( int itemCount = 1; ; itemCount++)
  {
    // reset MsgId/CorrelId each call, else BROWSE_NEXT tries to match the previous message.
    MQMD md = {MQMD_DEFAULT};
    mqlogctl_t ctrlMsg = MQLOGCTL_DEFAULT;
    MQLONG dataLen = 0;

    MQGET (conn->hConn, hCtrlQ, &md, &gmo,
           sizeof (ctrlMsg), &ctrlMsg, &dataLen, &mqcc, &mqrc);

    if( mqrc == MQRC_NO_MSG_AVAILABLE)
    {
      if (itemCount == 0)
        printf ("No daemon configuration entries found.\n");
      break;
    }

    if( mqrc != MQRC_NONE)
    {
      fprintf (stderr, "MQGET browse failed: CC=%d RC=%d %s\n", (int) mqcc, (int) mqrc, mqrc2str (mqrc));
      sysRc = -1;
      goto _door;
    }

    // qmgr is fixed-width and space-padded.
    char qmgr[MQ_Q_MGR_NAME_LENGTH + 1];
    memcpy (qmgr, ctrlMsg.qmgr, MQ_Q_MGR_NAME_LENGTH);
    qmgr[MQ_Q_MGR_NAME_LENGTH] = '\0';
    for (int i = MQ_Q_MGR_NAME_LENGTH - 1; i >= 0 && qmgr[i] == ' '; --i)
      qmgr[i] = '\0';

    // md.Expiry is the remaining time-to-live in tenths of a second.
    char expiryStr[20];
    if (md.Expiry == MQEI_UNLIMITED)
      strcpy (expiryStr, "never");
    else
    {
      time_t expiryTime = time (NULL) + (time_t) (md.Expiry / 10);
      struct tm *expiryTm = localtime (&expiryTime);

      strftime (expiryStr, sizeof (expiryStr), "%Y.%m.%d %H:%M", expiryTm);
    }

    printf ("%d:\n"
            "  structId        %.4s\n"
            "  version         %u\n"
            "  expiry          %s\n"
            "  collectionStop  %ld\n"
            "  collectionStart %ld\n"
            "  retryCount      %u\n"
            "  qmgr            %s\n"
            "  lastLogEntry    %.*s\n"
            "  fileName        %.*s\n",
            itemCount,
            ctrlMsg.structId,
            (unsigned int) ctrlMsg.version,
            expiryStr,
            (long) ctrlMsg.collectionStop,
            (long) ctrlMsg.collectionStart,
            (unsigned int) ctrlMsg.retryCount,
            qmgr[0] != '\0' ? qmgr : "(empty)",
            MQLOGCTL_ISO_TIME_LEN, ctrlMsg.lastLogEntry,
            MQLOGCTL_LOG_FILENAME_LEN, ctrlMsg.fileName);

    gmo.Options = MQGMO_BROWSE_NEXT | MQGMO_FAIL_IF_QUIESCING;
  }

  _door:
  if (hCtrlQ != MQHO_NONE && mqCloseQueue (conn, &hCtrlQ) != MQRC_NONE)
    sysRc = -1;
  return sysRc;
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
  // args.remQmgr is NULL/"" when -r/--remQmgr was not given,
  // in which case mqInquireErrorLog targets whichever queue
  // manager 'conn' is already connected to (the local one).
  // -------------------------------------------------------
  rc = mqInquireErrorLog (conn, args.remQmgr,
                          args.logFile, args.timeStamp,
                          args.format,
                          NULL);
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
  // Redirect normal output to the daemon log file.
  // -------------------------------------------------------
  gRealStdout = fdopen (dup (fileno (stdout)), "w");
  if (gRealStdout == NULL)
  {
    fprintf (stderr, "Error: cannot duplicate stdout\n");
    rc = -1;
    goto _door;
  }

  // -------------------------------------------------------
  // open control queue.
  // -------------------------------------------------------
  mqrc = mqOpenQueue( conn                ,
                      MQ_DISERR_CTRL_QUEUE,
                      NULL                ,
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

  while (1)
  {
    // -----------------------------------------------------
    // browse first message in the control queue 
    // -----------------------------------------------------
    MQMD   md = {MQMD_DEFAULT};
    MQGMO  gmo = {MQGMO_DEFAULT};
    MQLONG mqcc, dataLen;
  
    char amqerrFile[MQLOGCTL_LOG_FILENAME_LEN + 1];

    gmo.Options = MQGMO_BROWSE_FIRST | MQGMO_FAIL_IF_QUIESCING;
  
    mqlogctl_t ctrlMsg = MQLOGCTL_DEFAULT;
  
    MQGET (conn->hConn, hCtrlQ, &md, &gmo,
           sizeof (ctrlMsg), &ctrlMsg, &dataLen, &mqcc, &mqrc);
  
    switch (mqrc)
    {
      case MQRC_NONE:            // Success - continue.
        break;                   //
      case MQRC_NO_MSG_AVAILABLE: // No message - continue.
        break;                   //
      default:                   // Error - exit with failure.
        fprintf (stderr, "MQGET browse failed: CC=%d RC=%d %s\n", (int) mqcc, (int) mqrc, mqrc2str (mqrc));
        rc = -1;
        goto _door;
    }
    
    // -----------------------------------------------------
    // inquire logs on queue manager in control message
    // -----------------------------------------------------
    char nodeQmgr[MQ_Q_MGR_NAME_LENGTH + 1];
    size_t qmgrLen = strnlen (ctrlMsg.qmgr, sizeof (nodeQmgr));
    while (qmgrLen > 0 && ctrlMsg.qmgr[qmgrLen - 1] == ' ')
      qmgrLen--;
    snprintf (nodeQmgr, sizeof (nodeQmgr), "%.*s", (int) qmgrLen, ctrlMsg.qmgr);

    char outputLogPath[256];
    snprintf (outputLogPath, sizeof (outputLogPath), "/tmp/amqerr.%s.json",
              nodeQmgr[0] != '\0' ? nodeQmgr : "default");

    if (freopen (outputLogPath, "a", stdout) == NULL)
    {
      fprintf (stderr, "Error: cannot open daemon log file '%s'\n", outputLogPath);
      rc = -1;
      goto _door;
    }

    // -----------------------------------------------------
    // Initial run - last log entry is not set.
    // Get amqerr3, amqerr2 and amqerr1
    // and last log entry time
    // -----------------------------------------------------
    if( ctrlMsg.lastLogEntry[0] == '\0' )
    {
      for(int i = 3; i > 0; i--)
      {
        sprintf(amqerrFile, "AMQERR0%d", i);
        mqrc = mqInquireErrorLog( conn         , 
                                  ctrlMsg.qmgr , 
                                  amqerrFile   , 
                                  NULL         ,
                                  FMT_JSON     ,
                                  ctrlMsg.lastLogEntry );
        switch( mqrc )
        {
          case MQRC_NONE:             // Success - persist the progress.
            // -----------------------------------------------
            // consume a message identified by md->correlationId
            // put message with new controlStruct back to the control q
            // do both actions in a transactional manner
            // -----------------------------------------------
            ctrlMsg.retryCount = 0;
            if( mqUpdateCtrlMsg (conn, hCtrlQ, &md, &ctrlMsg,
                                 sizeof (ctrlMsg)) != MQRC_NONE )
              rc = -1;
            break;

          case MQRC_NO_MSG_AVAILABLE: 
            // -----------------------------------------------
            // No reply from command server 
            // requeue the control message at the tail of the queue
            // -----------------------------------------------
            ctrlMsg.retryCount++;
            ctrlMsg.collectionStart = time(NULL) + 100 + (ctrlMsg.retryCount * 30);
            if( mqRequeueCtrlMsg( conn, hCtrlQ, &md, &ctrlMsg,
                                  sizeof (ctrlMsg)) != MQRC_NONE )
              rc = -1;
            break;                    

          default:                    // Error - stop inquiring.
            rc = -1;
            break;
        }

        if( rc != 0 )
          break;  // leave the for loop; the switch break only left the switch

        // pace the PCF inquiries so the command server is not flooded
        struct timespec pause = {.tv_sec = 0, .tv_nsec = 100000000L};
        nanosleep (&pause, NULL);
      }
    }
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

  if (gRealStdout != NULL)
  {
    fclose (gRealStdout);
    gRealStdout = NULL;
  }

  return rc;
}
