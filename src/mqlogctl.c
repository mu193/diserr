/* mqlogctl.c - IBM MQ error log control functions.

   This module will contain functions for controlling and retrieving
   queue-manager error log files.

   Functions:
     addQmgr - register a queue manager in the daemon configuration
*/

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include "mqlogctl.h"
#include "cmdline.h"
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
