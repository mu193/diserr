/* mqlogctl.c - IBM MQ error log control functions.

   This module will contain functions for controlling and retrieving
   queue-manager error log files.

   Functions:
     addQmgr - register a queue manager in the daemon configuration
     listQmgr - list queue-manager entries from daemon configuration
*/

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include "mqlogctl.h"
#include "cmdline.h"
#include "mqreason.h"
#include <string.h>

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

    MQGET (conn->hConn, hCtrlQ, &md, &gmo,
           sizeof (ctrlMsg), &ctrlMsg, &dataLen, &mqcc, &mqrc);

    if (mqrc == MQRC_NO_MSG_AVAILABLE)
      break;

    if (mqrc != MQRC_NONE)
    {
      fprintf (stderr, "MQGET browse failed: CC=%d RC=%d %s\n", (int) mqcc, (int) mqrc, mqrc2str (mqrc));
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
  mqrc = mqOpenQueue (conn,
                     MQ_DISERR_CTRL_QUEUE,
                     NULL,
                     MQOO_BROWSE | MQOO_INQUIRE | MQOO_FAIL_IF_QUIESCING,
                     &hCtrlQ);
  if (mqrc != MQRC_NONE)
  {
    fprintf (stderr, "mqOpenQueue browse failed: RC=%d %s\n", (int) mqrc, mqrc2str (mqrc));
    sysRc = -1;
    goto _door;
  }

  MQGMO gmo = {MQGMO_DEFAULT};
  gmo.Options = MQGMO_BROWSE_FIRST | MQGMO_FAIL_IF_QUIESCING;

  int itemCount;
  for( itemCount = 1; ; itemCount++)
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
