/* mqutil.c - IBM MQ connection and utility functions.

   This module provides wrapper functions for IBM MQ operations,
   handling connections, disconnections, and error reporting.
   It uses the IBM MQ C API (cmqc.h) to communicate with queue managers.

   Functions:
     mqInit            - Initialize MQ connection structure
     mqConnect         - Connect to queue manager; auto-reconnect for client,
                         retry on transient QM unavailability for both modes
     mqDisconnect      - Disconnect from queue manager using MQDISC
     mqOpenQueue       - Open a queue using MQOPEN
     mqCloseQueue      - Close a queue opened with mqOpenQueue
     mqInquireErrorLog - Retrieve error log entries using PCF
*/

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <cmqc.h>
#include <cmqcfc.h>
#include <cmqbc.h>
#include "cmdline.h"
#include "mqconvert.h"
#include "osutil.h"
#include "mqreason.h"
#include "mqutil.h"

/******************************************************************************/
/*                               D E F I N E S                                */
/******************************************************************************/
#define MQ_1MB_BUF_LEN          (1024 * 1024)  /* 1 MB read buffer for PCF string values */
#define MQ_ADMIN_COMMAND_QUEUE   "SYSTEM.ADMIN.COMMAND.QUEUE"

/******************************************************************************/
/*                                                                            */
/*                             F U N C T I O N S                              */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/* Initialize MQ connection structure.                                        */
/******************************************************************************/
void mqInit (mqconn_t *conn)
{
  conn->hConn = MQHC_UNUSABLE_HCONN;
  conn->cc = MQCC_OK;
  conn->rc = MQRC_NONE;

  // -------------------------------------------------------
  // QM name from -m or -c (single field)
  // -------------------------------------------------------
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
/*                                                                            */
/* Client mode (-c): MQCNO_RECONNECT_Q_MGR instructs the MQ client library    */
/*   to transparently re-establish the connection if it drops. No application */
/*   code is needed; MQ handles retries and reconnection internally.          */
/*                                                                            */
/* Binding mode (-m): no MQ native auto-reconnect is available. Binding mode  */
/*   uses in-process shared memory; if the QM stops the connection is lost.   */
/******************************************************************************/
int mqConnect (mqconn_t *conn)
{
  MQCHAR qmgr[MQ_Q_MGR_NAME_LENGTH + 1];
  MQCNO  cno = {MQCNO_DEFAULT};  // Connection options struct.
  int    sysRc;

  cno.Version = MQCNO_CURRENT_VERSION;

  // -------------------------------------------------------
  // Prepare queue manager name. 
  // -------------------------------------------------------
  memset (qmgr, ' ', MQ_Q_MGR_NAME_LENGTH);
  qmgr[MQ_Q_MGR_NAME_LENGTH] = '\0';
  if (strlen (conn->qmgr) > 0)
      memcpy (qmgr, conn->qmgr, strlen (conn->qmgr));

  // -------------------------------------------------------
  // Set binding mode.                                      
  // Client: also request transparent auto-reconnect to the 
  // same QM if the connection drops during operation.      
  // -------------------------------------------------------
  switch (args.connType)
  {
    case CLIENT:
      cno.Options = MQCNO_CLIENT_BINDING ;
      break;
    case BINDING:
      cno.Options = MQCNO_STANDARD_BINDING;
      break;
  }

  MQCONNX (qmgr,
           &cno,
           &conn->hConn,
           &conn->cc,
           &conn->rc);

  switch (conn->rc)
  {
    case MQRC_NONE:
      if (args.verbose)
        printf ("Connected to queue manager: %s\n",
                strlen (conn->qmgr) > 0 ? conn->qmgr : "(default)");
      sysRc = 0;
      goto _door;

    default:
      fprintf (stderr, "MQCONNX failed with reason code %d: %s\n",
               (int) conn->rc,
               mqrc2str (conn->rc));
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
  int sysRc = 0;  // Assume success.

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
      if (args.verbose)
        printf ("Disconnected from queue manager\n");
      sysRc = 0;
      goto _door;

    default:
      fprintf (stderr, "MQDISC failed with reason code %d: %s\n",
               (int) conn->rc,
               mqrc2str (conn->rc));
      sysRc = conn->rc;
      goto _door;
  }

  _door:
    return sysRc;
}

/******************************************************************************/
/* Open a queue.                                                              */
/* Pass NULL or "" for rQmgr to target the local queue manager.               */
/******************************************************************************/
int mqOpenQueue ( mqconn_t   *conn   ,  // MQ connection handle and state
                  const char *qName  ,  // name of the queue to open
                  const char *rQmgr  ,  // queue manager (NULL/"" = local)
                  MQLONG      options ,  // MQOO_* open options
                  MQHOBJ     *hObj    )  // returned object handle
{
  MQOD   od = {MQOD_DEFAULT};  // Object descriptor: queue name and options
  MQLONG cc, rc;               // MQ completion code and reason code

  strncpy (od.ObjectName, qName, MQ_Q_NAME_LENGTH);

  if (rQmgr != NULL && strlen (rQmgr) > 0)
    strncpy (od.ObjectQMgrName, rQmgr, MQ_Q_MGR_NAME_LENGTH);

  MQOPEN (conn->hConn, &od, options, hObj, &cc, &rc);

  switch (rc)
  {
    case MQRC_NONE:   // Success - continue.
      break;          //
    default:          // Error - exit with failure.
      fprintf (stderr, "MQOPEN '%s'%s%s failed: CC=%d RC=%d %s\n",
               qName,
               (rQmgr != NULL && strlen (rQmgr) > 0) ? "@" : "",
               (rQmgr != NULL && strlen (rQmgr) > 0) ? rQmgr : "",
               (int) cc, (int) rc, mqrc2str (rc));
      break;
  }

  return rc;
}


/******************************************************************************/
/* Close a queue opened with mqOpenQueue.                                     */
/******************************************************************************/
int mqCloseQueue (mqconn_t *conn, MQHOBJ *hObj)
{
  MQLONG cc, rc;

  MQCLOSE (conn->hConn, hObj, MQCO_NONE, &cc, &rc);

  switch (rc)
  {
    case MQRC_NONE:
      break;
    default:
      fprintf (stderr, "MQCLOSE failed: CC=%d RC=%d %s\n",
               (int) cc, (int) rc, mqrc2str (rc));
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
  const MQCHAR *logName      = (args.logFile   != NULL) ? args.logFile   : "";
  const MQCHAR *afterIsoTime = (args.timeStamp != NULL) ? args.timeStamp : "";

  // timeStamp is already normalized to yyyy-mm-ddThh:mm:ss.sssZ by cmdlParse.

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
               (int) rc, mqrc2str (rc));
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
        fprintf (stderr, "mqAddString (log name) failed: %d %s\n",
                 (int) rc, mqrc2str (rc));
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
        fprintf (stderr, "mqAddString (after isotime) failed: %d %s\n",
                 (int) rc, mqrc2str (rc));
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
               (int) rc, mqrc2str (rc));
      sysRc = rc;
      goto _cleanup;
  }

  // -------------------------------------------------------
  // Open SYSTEM.ADMIN.COMMAND.QUEUE for PCF execution.
  // If -r / --remQmgr was specified, the queue is opened
  // with ObjectQMgrName set to that remote queue manager,
  // routing the PCF command over the MQ channel infrastructure.
  // If no remote QM was given, NULL is passed and the command
  // targets the local queue manager.
  // -------------------------------------------------------
  MQHOBJ hAdminQ = MQHO_NONE;

  if (mqOpenQueue (conn, MQ_ADMIN_COMMAND_QUEUE, args.remQmgr,
                   MQOO_OUTPUT, &hAdminQ) != MQRC_NONE)
  {
    sysRc = 1;
    goto _cleanup;
  }

  // -------------------------------------------------------
  // Execute PCF: MQCMD_INQUIRE_ERROR_LOG command.  
  // -------------------------------------------------------
  mqExecute (conn->hConn,
             MQCMD_INQUIRE_ERROR_LOG,
             MQHB_NONE,
             cmdBag,
             repBag,
             hAdminQ,
             MQHO_NONE,
             &cc, &rc);

  switch (rc)
  {
    case MQRC_NONE:
      break;

    default:            // Extract actual reason from the response bag.
    {
      // Save the mqExecute return codes before bag operations overwrite them.
      MQLONG execCC = cc;
      MQLONG execRC = rc;

      MQHBAG errorBag  = MQHB_UNUSABLE_HBAG;
      MQLONG cmdSrvCC  = execCC;   // fallback: use mqExecute codes
      MQLONG cmdSrvRC  = execRC;
      MQLONG bagCC, bagRC;

      // Try to read the command server's own cc/rc from the response bag.
      // This only works when mqExecute got a reply back (command server
      // error); it does NOT work for transport-level failures where the
      // response bag was never populated.
      mqInquireBag     (repBag,    MQHA_BAG_HANDLE,   0,          &errorBag, &bagCC, &bagRC);
      if (bagRC == MQRC_NONE)
      {
        mqInquireInteger (errorBag, MQIASY_COMP_CODE,  MQIND_NONE, &cmdSrvCC, &bagCC, &bagRC);
        mqInquireInteger (errorBag, MQIASY_REASON,     MQIND_NONE, &cmdSrvRC, &bagCC, &bagRC);
      }

      switch (cmdSrvRC)
      {
        case MQRCCF_NONE_FOUND:           // No entries match the criteria - not an error.
          printf ("No error log entries found for the given criteria.\n");
          sysRc = 0;
          break;

        default:                          // Genuine command server error.
          fprintf (stderr, "mqExecute failed: command server cc=%d rc=%d %s\n",
                   (int) cmdSrvCC, (int) cmdSrvRC, mqrc2str (cmdSrvRC));
          sysRc = cmdSrvRC;
          break;
      }
      goto _cleanup;
    }
  }

  // -------------------------------------------------------
  // Process response bags.                                
  //   Each embedded bag holds one PCF response message.  
  // -------------------------------------------------------
  MQLONG bagCount;
  mqCountItems (repBag, MQHA_BAG_HANDLE, &bagCount, &cc, &rc);
  switch (rc)
  {
    case MQRC_NONE:
      break;
    default:
      fprintf (stderr, "mqCountItems failed: %d %s\n",
               (int) rc, mqrc2str (rc));
      sysRc = rc;
      goto _cleanup;
  }

  // -------------------------------------------------------
  // Iterate over each embedded bag in the response bag.
  // -------------------------------------------------------
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
          MQCHAR value[MQ_1MB_BUF_LEN];
          MQLONG strLen;
          mqInquireString (itemBag, selector, 0,
                           sizeof (value), value, &strLen,
                           NULL, &cc, &rc);
          switch (rc)
          {
            case MQRC_NONE:
            {
              const char *sname = mqselector2str ((int) selector);
              if (args.verbose)
                printf ("  [%s] %.*s\n", sname, (int) strLen, value);
              else if (selector == MQCACF_ERROR_LOG_RECORD ||
                       selector == MQCACF_ERROR_LOG_ISOTIME)
              {
                switch (args.format)
                {
                  case FMT_TEXT:
                    printf ("%.*s\n", (int) strLen, value);
                    break;

                  case FMT_JSON:
                  {
                    char *line = malloc ((size_t) strLen + 1);
                    char *json = NULL;
                    if (line == NULL)
                    {
                      fprintf (stderr, "Error: Memory allocation failed\n");
                      sysRc = 1;
                      goto _cleanup;
                    }

                    memcpy (line, value, (size_t) strLen);
                    line[strLen] = '\0';

                    if (osutilMqrcTextToJson (line, &json) != 0)
                    {
                      fprintf (stderr, "Error: failed to convert output to JSON\n");
                      free (line);
                      sysRc = 1;
                      goto _cleanup;
                    }

                    free (line);
                    if (json != NULL)
                    {
                      fputs (json, stdout);
                      if (json[0] != '\0' && json[strlen (json) - 1] != '\n')
                        fputc ('\n', stdout);
                      free (json);
                    }
                    break;
                  }

                  default:
                    fprintf (stderr, "Error: unsupported output format\n");
                    sysRc = 1;
                    goto _cleanup;
                }
              }
              break;
            }
            case MQRC_STRING_TRUNCATED:
              fprintf (stderr, "  Warning: selector %d string truncated"
                               " (%d bytes returned)\n",
                       (int) selector, (int) sizeof (value));
              printf ("  [%d] %.*s\n", (int) selector,
                      (int) sizeof (value), value);
              break;
            default:
              fprintf (stderr, "  mqInquireString selector %d failed: %d %s\n",
                       (int) selector, (int) rc, mqrc2str (rc));
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
            {
              const char *sname = mqselector2str ((int) selector);
              if (args.verbose)
                printf ("  [%s] %d\n", sname, (int) value);
              break;
            }
            default:
              fprintf (stderr, "  mqInquireInteger selector %d failed: %d %s\n",
                       (int) selector, (int) rc, mqrc2str (rc));
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
    if (hAdminQ != MQHO_NONE)          mqCloseQueue  (conn, &hAdminQ);
    if (cmdBag  != MQHB_UNUSABLE_HBAG) mqDeleteBag (&cmdBag, &cc, &rc);
    if (repBag  != MQHB_UNUSABLE_HBAG) mqDeleteBag (&repBag, &cc, &rc);

  return sysRc;
}

