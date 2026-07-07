/* mqutil.c - IBM MQ connection and utility functions.

   This module provides wrapper functions for IBM MQ operations,
   handling connections, disconnections, and error reporting.
   It uses the IBM MQ C API (cmqc.h) to communicate with queue managers.

   Functions:
     mqInit       - Initialize MQ connection structure
     mqConnect    - Connect to IBM MQ queue manager using MQCONN
     mqDisconnect - Disconnect from queue manager using MQDISC
     mqReasonStr  - Convert MQ reason codes to readable strings
*/

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmqc.h>
#include "mqutil.h"

/******************************************************************************/
/* Initialize MQ connection structure.                                        */
/******************************************************************************/
void mqInit (mqconn_t *conn, const char *qmgr)
{
  conn->hConn = MQHC_UNUSABLE_HCONN;
  conn->cc = MQCC_OK;
  conn->rc = MQRC_NONE;

  if (qmgr != NULL)
  {
      strncpy (conn->qmgr, qmgr, MQ_Q_MGR_NAME_LENGTH);
      conn->qmgr[MQ_Q_MGR_NAME_LENGTH] = '\0';
  }
  else
  {
      conn->qmgr[0] = '\0';
  }
}

/* Connect to IBM MQ queue manager.  */
int mqConnect (mqconn_t *conn)
{
  MQCHAR qmgr[MQ_Q_MGR_NAME_LENGTH + 1];

  /* Prepare queue manager name.  */
  memset (qmgr, ' ', MQ_Q_MGR_NAME_LENGTH);
  if (strlen (conn->qmgr) > 0)
  {
      memcpy (qmgr, conn->qmgr, strlen (conn->qmgr));
  }

  /* Connect to queue manager.  */
  MQCONN (qmgr,
          &conn->hConn,
          &conn->cc,
          &conn->rc);

  if (conn->cc == MQCC_FAILED)
  {
      fprintf (stderr, "MQCONN failed with reason code %d: %s\n",
               (int) conn->rc,
               mqReasonStr (conn->rc));
      return -1;
  }

  if (conn->cc == MQCC_WARNING)
  {
      fprintf (stderr, "MQCONN completed with warning %d: %s\n",
               (int) conn->rc,
               mqReasonStr (conn->rc));
  }

  printf ("Connected to queue manager: %s\n",
          strlen (conn->qmgr) > 0 ? conn->qmgr : "(default)");

  return 0;
}

/* Disconnect from IBM MQ queue manager.  */
int mqDisconnect (mqconn_t *conn)
{
  if (conn->hConn == MQHC_UNUSABLE_HCONN)
  {
      return 0;  /* Already disconnected.  */
  }

  MQDISC (&conn->hConn,
          &conn->cc,
          &conn->rc);

  if (conn->cc == MQCC_FAILED)
  {
      fprintf (stderr, "MQDISC failed with reason code %d: %s\n",
               (int) conn->rc,
               mqReasonStr (conn->rc));
      return -1;
  }

  printf ("Disconnected from queue manager\n");

  return 0;
}

/* Get human-readable string for MQ reason code.  */
const char *mqReasonStr (MQLONG reason_code)
{
  switch (reason_code)
    {
    case MQRC_NONE:
      return "No reason";
    case MQRC_Q_MGR_NAME_ERROR:
      return "Queue manager name error";
    case MQRC_Q_MGR_NOT_AVAILABLE:
      return "Queue manager not available";
    case MQRC_CONNECTION_BROKEN:
      return "Connection broken";
    case MQRC_HCONN_ERROR:
      return "Connection handle error";
    case MQRC_Q_MGR_STOPPING:
      return "Queue manager stopping";
    case MQRC_Q_MGR_QUIESCING:
      return "Queue manager quiescing";
    case MQRC_CONNECTION_QUIESCING:
      return "Connection quiescing";
    case MQRC_CONNECTION_NOT_AUTHORIZED:
      return "Connection not authorized";
    case MQRC_ALREADY_CONNECTED:
      return "Already connected";
    default:
      return "Unknown reason code";
    }
}
