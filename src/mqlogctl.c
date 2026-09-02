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
  int mqrc;
  int rc = 0;

  // -------------------------------------------------------
  // open control queue for put.
  // -------------------------------------------------------
  mqrc = mqOpenQueue( conn                ,
                      MQ_DISERR_CTRL_QUEUE,
                      NULL                ,
                      MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING,
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
  // put initial control message to the queue.
  // -------------------------------------------------------
  mqlogctl_t ctrlMsg = MQLOGCTL_DEFAULT;
  size_t nodeQmgrLen = strlen (args.nodeQmgr);

  memset (ctrlMsg.qmgr, ' ', sizeof (ctrlMsg.qmgr));
  memcpy (ctrlMsg.qmgr, args.nodeQmgr, nodeQmgrLen);

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
  int itemCount = 0;

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

  MQMD md = {MQMD_DEFAULT};
  MQGMO gmo = {MQGMO_DEFAULT};
  gmo.Options = MQGMO_BROWSE_FIRST | MQGMO_FAIL_IF_QUIESCING;

  for (;;)
  {
    mqlogctl_t ctrlMsg = MQLOGCTL_DEFAULT;
    MQLONG dataLen = 0;

    MQGET (conn->hConn, hCtrlQ, &md, &gmo,
           sizeof (ctrlMsg), &ctrlMsg, &dataLen, &mqcc, &mqrc);

    if( mqrc == MQRC_NO_MSG_AVAILABLE)
      break;

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

    printf ("%d: qmgr=%s file=%.*s retry=%u\n",
            ++itemCount,
            qmgr[0] != '\0' ? qmgr : "(empty)",
            MQLOGCTL_LOG_FILENAME_LEN, ctrlMsg.fileName,
            (unsigned int) ctrlMsg.retryCount);

    gmo.Options = MQGMO_BROWSE_NEXT | MQGMO_FAIL_IF_QUIESCING;
  }

  if (itemCount == 0)
    printf ("No daemon configuration entries found.\n");

  _door:
  if (hCtrlQ != MQHO_NONE && mqCloseQueue (conn, &hCtrlQ) != MQRC_NONE)
    sysRc = -1;
  return sysRc;
}
