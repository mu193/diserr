/* mqutil.c - IBM MQ connection and utility functions.

   This module provides wrapper functions for IBM MQ operations,
   handling connections, disconnections, and error reporting.
   It uses the IBM MQ C API (cmqc.h) to communicate with queue managers.

   Functions:
     mqInit            - Initialize MQ connection structure
     mqConnect         - Connect to IBM MQ queue manager using MQCONN
     mqDisconnect      - Disconnect from queue manager using MQDISC
     mqReasonStr       - Convert MQ reason codes to readable strings
     mqOpenQueue       - Open a queue using MQOPEN
     mqInquireErrorLog - Retrieve error log entries using PCF
*/

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cmqc.h>
#include <cmqcfc.h>
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

/******************************************************************************/
/* Connect to IBM MQ queue manager.                                           */
/******************************************************************************/
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

/******************************************************************************/
/* Disconnect from IBM MQ queue manager.                                      */
/******************************************************************************/
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

/******************************************************************************/
/* Get human-readable string for MQ reason code.                            */
/******************************************************************************/
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
    case MQRC_NOT_AUTHORIZED:
      return "Not authorized";
    case MQRC_UNKNOWN_OBJECT_NAME:
      return "Unknown object name";
    case MQRC_OBJECT_IN_USE:
      return "Object in use";
    case MQRC_OBJECT_TYPE_ERROR:
      return "Object type error";
    case MQRC_HOBJ_ERROR:
      return "Object handle error";
    case MQRC_Q_FULL:
      return "Queue full";
    case MQRC_NO_MSG_AVAILABLE:
      return "No message available";
    case MQRC_TRUNCATED_MSG_FAILED:
      return "Truncated message";
    case MQRC_TRUNCATED_MSG_ACCEPTED:
      return "Truncated message accepted";
    case MQRC_OPTIONS_ERROR:
      return "Options error";
    case MQRC_SELECTOR_ERROR:
      return "Selector error";
    case MQRC_OBJECT_DAMAGED:
      return "Object damaged";
    case MQRC_STORAGE_NOT_AVAILABLE:
      return "Storage not available";
    case MQRC_BUFFER_ERROR:
      return "Buffer error";
    case MQRC_BUFFER_LENGTH_ERROR:
      return "Buffer length error";
    case MQRC_INSUFFICIENT_BUFFER:
      return "Insufficient buffer";
    case MQRC_CMD_SERVER_NOT_AVAILABLE:
      return "Command server not available";
    case MQRC_MSG_TOO_BIG_FOR_Q:
      return "Message too big for queue";
    default:
      return "Unknown reason code";
    }
}


/******************************************************************************/
/* Open a queue.                                                              */
/******************************************************************************/
int mqOpenQueue (mqconn_t *conn, const char *qname, MQLONG options, MQHOBJ *hObj)
{
  MQOD   od = {MQOD_DEFAULT};
  MQLONG cc, rc;

  strncpy (od.ObjectName, qname, MQ_Q_NAME_LENGTH);

  MQOPEN (conn->hConn, &od, options, hObj, &cc, &rc);

  if (cc == MQCC_FAILED)
  {
      fprintf (stderr, "MQOPEN '%s' failed: %d %s\n",
               qname, (int) rc, mqReasonStr (rc));
      return -1;
  }

  return 0;
}


/******************************************************************************/
/* Inquire error log entries from the queue manager via PCF.                  */
/******************************************************************************/
int mqInquireErrorLog (mqconn_t *conn)
{
  MQHOBJ  hCmd   = MQHO_UNUSABLE_HOBJ;
  MQHOBJ  hReply = MQHO_UNUSABLE_HOBJ;
  MQOD    cmdOD  = {MQOD_DEFAULT};
  MQOD    repOD  = {MQOD_DEFAULT};
  MQMD    md     = {MQMD_DEFAULT};
  MQPMO   pmo    = {MQPMO_DEFAULT};
  MQGMO   gmo    = {MQGMO_DEFAULT};
  MQLONG  cc, rc;
  MQCFH  *pCFH;
  MQBYTE  reqBuf[MQCFH_STRUC_LENGTH];
  MQBYTE *repBuf = NULL;
  MQLONG  repLen = 65536;
  MQLONG  msgLen;
  int     ret    = 0;

  /* Open command input queue.  */
  strncpy (cmdOD.ObjectName, "SYSTEM.ADMIN.COMMAND.QUEUE",
           MQ_Q_NAME_LENGTH);
  MQOPEN (conn->hConn, &cmdOD,
          MQOO_OUTPUT | MQOO_FAIL_IF_QUIESCING,
          &hCmd, &cc, &rc);
  if (cc == MQCC_FAILED)
  {
      fprintf (stderr, "MQOPEN command queue failed: %d %s\n",
               (int) rc, mqReasonStr (rc));
      return -1;
  }

  /* Open temporary dynamic reply queue.  */
  strncpy (repOD.ObjectName,   "SYSTEM.DEFAULT.MODEL.QUEUE", MQ_Q_NAME_LENGTH);
  strncpy (repOD.DynamicQName, "DISERR.*",                   MQ_Q_NAME_LENGTH);
  MQOPEN (conn->hConn, &repOD,
          MQOO_INPUT_EXCLUSIVE | MQOO_FAIL_IF_QUIESCING,
          &hReply, &cc, &rc);
  if (cc == MQCC_FAILED)
  {
      fprintf (stderr, "MQOPEN reply queue failed: %d %s\n",
               (int) rc, mqReasonStr (rc));
      ret = -1;
      goto cleanup;
  }

  /* Build PCF MQCMD_INQUIRE_ERROR_LOG request.  */
  memset (reqBuf, 0, sizeof (reqBuf));
  pCFH                 = (MQCFH *) reqBuf;
  pCFH->Type           = MQCFT_COMMAND;
  pCFH->StrucLength    = MQCFH_STRUC_LENGTH;
  pCFH->Version        = MQCFH_VERSION_1;
  pCFH->Command        = MQCMD_INQUIRE_ERROR_LOG;
  pCFH->MsgSeqNumber   = 1;
  pCFH->Control        = MQCFC_LAST;
  pCFH->ParameterCount = 0;

  /* Set up message descriptor.  */
  memcpy (md.Format, MQFMT_ADMIN, MQ_FORMAT_LENGTH);
  md.MsgType = MQMT_REQUEST;
  memcpy (md.ReplyToQ, repOD.ObjectName, MQ_Q_NAME_LENGTH);

  /* Put request to command queue.  */
  pmo.Options = MQPMO_NO_SYNCPOINT | MQPMO_FAIL_IF_QUIESCING;
  MQPUT (conn->hConn, hCmd, &md, &pmo,
         MQCFH_STRUC_LENGTH, reqBuf, &cc, &rc);
  if (cc == MQCC_FAILED)
  {
      fprintf (stderr, "MQPUT failed: %d %s\n",
               (int) rc, mqReasonStr (rc));
      ret = -1;
      goto cleanup;
  }

  /* Receive and print response messages.  */
  repBuf = malloc (repLen);
  if (repBuf == NULL)
  {
      fprintf (stderr, "Error: memory allocation failed\n");
      ret = -1;
      goto cleanup;
  }

  gmo.Options      = MQGMO_NO_SYNCPOINT | MQGMO_WAIT | MQGMO_CONVERT;
  gmo.WaitInterval = 5000;

  do
  {
      MQMD repMD = {MQMD_DEFAULT};

      MQGET (conn->hConn, hReply, &repMD, &gmo,
             repLen, repBuf, &msgLen, &cc, &rc);
      if (cc == MQCC_FAILED)
      {
          if (rc != MQRC_NO_MSG_AVAILABLE)
          {
              fprintf (stderr, "MQGET failed: %d %s\n",
                       (int) rc, mqReasonStr (rc));
              ret = -1;
          }
          break;
      }

      pCFH = (MQCFH *) repBuf;

      if (pCFH->Reason != MQRC_NONE)
      {
          fprintf (stderr, "Error log inquiry failed: %d %s\n",
                   (int) pCFH->Reason, mqReasonStr (pCFH->Reason));
          ret = -1;
          break;
      }

      /* Iterate PCF parameters and print values.  */
      MQBYTE *p     = repBuf + MQCFH_STRUC_LENGTH;
      MQLONG  count = pCFH->ParameterCount;

      for (MQLONG i = 0; i < count; i++)
      {
          MQLONG type = ((MQCFIN *) p)->Type;

          if (type == MQCFT_STRING)
          {
              MQCFST *pStr = (MQCFST *) p;
              printf ("  %.*s\n", (int) pStr->StringLength, pStr->String);
              p += pStr->StrucLength;
          }
          else if (type == MQCFT_INTEGER)
          {
              MQCFIN *pInt = (MQCFIN *) p;
              printf ("  [%d] %d\n", (int) pInt->Parameter, (int) pInt->Value);
              p += pInt->StrucLength;
          }
          else
          {
              break;
          }
      }
  }
  while (pCFH->Control != MQCFC_LAST);

cleanup:
  free (repBuf);
  if (hReply != MQHO_UNUSABLE_HOBJ)
      MQCLOSE (conn->hConn, &hReply, MQCO_NONE, &cc, &rc);
  if (hCmd != MQHO_UNUSABLE_HOBJ)
      MQCLOSE (conn->hConn, &hCmd,   MQCO_NONE, &cc, &rc);

  return ret;
}

