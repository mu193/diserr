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
#include <cmqbc.h>
#include "cmdline.h"
#include "mqutil.h"

/******************************************************************************/
/* Initialize MQ connection structure.                                        */
/******************************************************************************/
void mqInit (mqconn_t *conn)
{
  conn->hConn = MQHC_UNUSABLE_HCONN;
  conn->cc = MQCC_OK;
  conn->rc = MQRC_NONE;

  if (args.qmgr != NULL)
  {
      strncpy (conn->qmgr, args.qmgr, MQ_Q_MGR_NAME_LENGTH);
      conn->qmgr[MQ_Q_MGR_NAME_LENGTH] = '\0';
  }
  else
  {
      conn->qmgr[0] = '\0';
  }
}

/******************************************************************************/
/* Connect to the queue manager.                                              */
/******************************************************************************/
int mqConnect (mqconn_t *conn)
{
  MQCHAR qmgr[MQ_Q_MGR_NAME_LENGTH + 1];

  int sysRc ;

  // -------------------------------------------------------
  // Prepare queue manager name. 
  // -------------------------------------------------------
  memset (qmgr, ' ', MQ_Q_MGR_NAME_LENGTH);
  if (strlen (conn->qmgr) > 0)
  {
      memcpy (qmgr, conn->qmgr, strlen (conn->qmgr));
  }

  // -------------------------------------------------------
  // Connect to queue manager.  
  // -------------------------------------------------------
  MQCONN (qmgr,
          &conn->hConn,
          &conn->cc,
          &conn->rc);

  switch (conn->rc)
  {
    case MQRC_NONE:
      printf ("Connected to queue manager: %s\n",
              strlen (conn->qmgr) > 0 ? conn->qmgr : "(default)");
      sysRc = 0;
      goto _door;

    default:
      fprintf (stderr, "MQCONN failed with reason code %d: %s\n",
               (int) conn->rc,
               mqReasonStr (conn->rc));
      sysRc = conn->rc;
      goto _door;
  }

  _door:
    return sysRc;
}

/******************************************************************************/
/* Disconnect from IBM MQ queue manager.                                      */
/******************************************************************************/
int mqDisconnect (mqconn_t *conn)
{
  int sysRc = 0 ; // let assume ok

  if (conn->hConn == MQHC_UNUSABLE_HCONN)
  {
      sysRc = 0;
      goto _door;
  }

  MQDISC (&conn->hConn,
          &conn->cc,
          &conn->rc);

  switch (conn->rc)
  {
    case MQRC_NONE:
      printf ("Disconnected from queue manager\n");
      sysRc = 0;
      goto _door;

    default:
      fprintf (stderr, "MQDISC failed with reason code %d: %s\n",
               (int) conn->rc,
               mqReasonStr (conn->rc));
      sysRc = conn->rc;
      goto _door;
  }

  _door:
    return sysRc;
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

  switch (rc)
  {
    case MQRC_NONE:   // Success - continue.
      break;          //
    default:          // Error - exit with failure.
      fprintf (stderr, "MQOPEN '%s' failed: CC=%d RC=%d %s\n",
               qname, (int) cc, (int) rc, mqReasonStr (rc));
      break;
  }

  return rc;
}


/******************************************************************************/
/* Inquire error log entries from the queue manager via MQAI Bags.            */
/******************************************************************************/
int mqInquireErrorLog (mqconn_t *conn)
{
  // -------------------------------------------------------
  // Request parameters from command line args.
  // -------------------------------------------------------
  const MQCHAR *logName      = (args.logfile   != NULL) ? args.logfile   : "";
  const MQCHAR *afterIsoTime = (args.timestamp != NULL) ? args.timestamp : "";

  MQHBAG  cmdBag = MQHB_UNUSABLE_HBAG;
  MQHBAG  repBag = MQHB_UNUSABLE_HBAG;
  MQLONG  cc, rc;
  int     sysRc  = 0;

  // -------------------------------------------------------
  // Create command bag.  
  // -------------------------------------------------------
  mqCreateBag (MQCBO_ADMIN_BAG, &cmdBag, &cc, &rc);
  switch (rc)
  {
    case MQRC_NONE:
      break;
    default:
      fprintf (stderr, "mqCreateBag (command) failed: %d %s\n",
               (int) rc, mqReasonStr (rc));
      sysRc = rc;
      goto _cleanup;
  }

  // -------------------------------------------------------
  // Add optional request parameters to command bag.
  // -------------------------------------------------------
  if (strlen (logName) > 0)
  {
    mqAddString (cmdBag, MQCACF_ERROR_LOG_NAME,
                 MQBL_NULL_TERMINATED, (MQCHAR *) logName,
                 &cc, &rc);
    switch (rc)
    {
      case MQRC_NONE:
        break;
      default:
        fprintf( stderr, "mqAddString (log name) failed: %d %s\n",
                         (int) rc, mqReasonStr (rc));
        sysRc = rc;
        goto _cleanup;
    }
  }

  if (strlen (afterIsoTime) > 0)
  {
    mqAddString (cmdBag, MQCACF_ERROR_LOG_AFTER_ISOTIME,
                   MQBL_NULL_TERMINATED, (MQCHAR *) afterIsoTime,
                   &cc, &rc);
    switch (rc)
    {
      case MQRC_NONE:
        break;
      default:
        fprintf ( stderr, "mqAddString (after isotime) failed: %d %s\n",
                  (int) rc, mqReasonStr (rc));
          sysRc = rc;
          goto _cleanup;
    }
  }

  // -------------------------------------------------------
  // Create response bag. 
  // -------------------------------------------------------
  mqCreateBag (MQCBO_ADMIN_BAG, &repBag, &cc, &rc);
  switch (rc)
  {
    case MQRC_NONE:
      break;
    default:
      fprintf (stderr, "mqCreateBag (response) failed: %d %s\n",
               (int) rc, mqReasonStr (rc));
      sysRc = rc;
      goto _cleanup;
  }

  //-------------------------------------------------------
  // Execute PCF: MQCMD_INQUIRE_ERROR_LOG command.  
  //-------------------------------------------------------
  mqExecute (conn->hConn,
             MQCMD_INQUIRE_ERROR_LOG,
             MQHB_NONE,
             cmdBag,
             repBag,
             MQHO_NONE,
             MQHO_NONE,
             &cc, &rc);
  switch (rc)
  {
    case MQRC_NONE:
      break;
    default:            // Extract actual error reason 
    {                   // from the response bag system bag.  
      MQHBAG errorBag;
      MQLONG cmdSrvCC, cmdSrvRC;
      mqInquireBag (repBag, MQHA_BAG_HANDLE, 0, &errorBag, &cc, &rc);
      mqInquireInteger (errorBag, MQIASY_COMP_CODE, MQIND_NONE,
                        &cmdSrvCC, &cc, &rc);
      mqInquireInteger (errorBag, MQIASY_REASON, MQIND_NONE,
                        &cmdSrvRC, &cc, &rc);
      fprintf (stderr, "mqExecute failed: command server cc=%d rc=%d %s\n",
               (int) cmdSrvCC, (int) cmdSrvRC, mqReasonStr (cmdSrvRC));
      sysRc = cmdSrvRC;
      goto _cleanup;
    }
  }

  //-------------------------------------------------------
  // Handle response bag contains 
  //   it contains one embedded bag per PCF response message.  
  //-------------------------------------------------------
  MQLONG bagCount;
  mqCountItems (repBag, MQHA_BAG_HANDLE, &bagCount, &cc, &rc);
  switch (rc)
  {
    case MQRC_NONE:
      break;
    default:
      fprintf (stderr, "mqCountItems failed: %d %s\n",
               (int) rc, mqReasonStr (rc));
      sysRc = rc;
      goto _cleanup;
  }

  //-------------------------------------------------------
  // Iterate over each embedded bag in the response bag.
  //-------------------------------------------------------
  for (MQLONG b = 0; b < bagCount; b++)
  {
    MQHBAG itemBag;
    mqInquireBag (repBag, MQHA_BAG_HANDLE, b, &itemBag, &cc, &rc);
    if (rc != MQRC_NONE) break;

    MQLONG paramCount;
    mqCountItems (itemBag, MQSEL_ALL_SELECTORS, &paramCount, &cc, &rc);
    if (rc != MQRC_NONE) break;

    for (MQLONG i = 0; i < paramCount; i++)
    {
      MQLONG selector, itemType;
      mqInquireItemInfo (itemBag, MQSEL_ALL_SELECTORS, i,
                         &selector, &itemType, &cc, &rc);
      if (rc != MQRC_NONE) break;

      switch (itemType)
      {
        // -------------------------------------------------
        // Handle STRING item types.
        // -------------------------------------------------
        case MQIT_STRING:
        {
          MQCHAR value[1048576];
          MQLONG strLen;
          mqInquireString (itemBag, selector, 0,
                           sizeof (value), value, &strLen,
                           NULL, &cc, &rc);
          switch (rc)
          {
            case MQRC_NONE:
              printf ("  [%d] %.*s\n", (int) selector,
                      (int) strLen, value);
              break;
            case MQRC_STRING_TRUNCATED:
              fprintf (stderr, "  Warning: selector %d string truncated"
                               " (%d bytes returned)\n",
                       (int) selector, (int) sizeof (value));
              printf ("  [%d] %.*s\n", (int) selector,
                      (int) sizeof (value), value);
              break;
            default:
              fprintf (stderr, "  mqInquireString selector %d failed: %d %s\n",
                       (int) selector, (int) rc, mqReasonStr (rc));
              break;
          }
          break;
        }

        // -------------------------------------------------
        // Handle INTEGER item types.
        // -------------------------------------------------
        case MQIT_INTEGER:
        {
          MQLONG value;
          mqInquireInteger (itemBag, selector, 0,
                            &value, &cc, &rc);
          switch (rc)
          {
            case MQRC_NONE:
              printf ("  [%d] %d\n", (int) selector, (int) value);
              break;
            default:
              fprintf (stderr, "  mqInquireInteger selector %d failed: %d %s\n",
                       (int) selector, (int) rc, mqReasonStr (rc));
              break;
          }
          break;
        }

        // -------------------------------------------------
        // Any other item types are unexpected for this command.
        // -------------------------------------------------
        default:
          fprintf (stderr, "Warning: unexpected item type %d for selector %d\n",
                   (int) itemType, (int) selector);
          break;
      }
    }
  }

  _cleanup:
    if (cmdBag != MQHB_UNUSABLE_HBAG) mqDeleteBag (&cmdBag, &cc, &rc);
    if (repBag != MQHB_UNUSABLE_HBAG) mqDeleteBag (&repBag, &cc, &rc);

  return sysRc;
}

