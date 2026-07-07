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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmqc.h>
#include "mqutil.h"

/* Initialize MQ connection structure.  */
void mqInit (mqconn_t *mq_conn, const char *qmgr_name)
{
  mq_conn->hConn = MQHC_UNUSABLE_HCONN;
  mq_conn->comp_code = MQCC_OK;
  mq_conn->reason_code = MQRC_NONE;

  if (qmgr_name != NULL)
  {
      strncpy (mq_conn->qmgr_name, qmgr_name, MQ_Q_MGR_NAME_LENGTH);
      mq_conn->qmgr_name[MQ_Q_MGR_NAME_LENGTH] = '\0';
  }
  else
  {
      mq_conn->qmgr_name[0] = '\0';
  }
}

/* Connect to IBM MQ queue manager.  */
int mqConnect (mqconn_t *mq_conn)
{
  MQCHAR qmgr_name[MQ_Q_MGR_NAME_LENGTH + 1];

  /* Prepare queue manager name.  */
  memset (qmgr_name, ' ', MQ_Q_MGR_NAME_LENGTH);
  if (strlen (mq_conn->qmgr_name) > 0)
  {
      memcpy (qmgr_name, mq_conn->qmgr_name, strlen (mq_conn->qmgr_name));
  }

  /* Connect to queue manager.  */
  MQCONN (qmgr_name,
          &mq_conn->hConn,
          &mq_conn->comp_code,
          &mq_conn->reason_code);

  if (mq_conn->comp_code == MQCC_FAILED)
  {
      fprintf (stderr, "MQCONN failed with reason code %d: %s\n",
               (int) mq_conn->reason_code,
               mqReasonStr (mq_conn->reason_code));
      return -1;
  }

  if (mq_conn->comp_code == MQCC_WARNING)
  {
      fprintf (stderr, "MQCONN completed with warning %d: %s\n",
               (int) mq_conn->reason_code,
               mqReasonStr (mq_conn->reason_code));
  }

  printf ("Connected to queue manager: %s\n",
          strlen (mq_conn->qmgr_name) > 0 ? mq_conn->qmgr_name : "(default)");

  return 0;
}

/* Disconnect from IBM MQ queue manager.  */
int mqDisconnect (mqconn_t *mq_conn)
{
  if (mq_conn->hConn == MQHC_UNUSABLE_HCONN)
  {
      return 0;  /* Already disconnected.  */
  }

  MQDISC (&mq_conn->hConn,
          &mq_conn->comp_code,
          &mq_conn->reason_code);

  if (mq_conn->comp_code == MQCC_FAILED)
  {
      fprintf (stderr, "MQDISC failed with reason code %d: %s\n",
               (int) mq_conn->reason_code,
               mqReasonStr (mq_conn->reason_code));
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
