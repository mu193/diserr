#ifndef MQUTIL_H
#define MQUTIL_H

#include <cmqc.h>

/******************************************************************************/
/* MQ Connection structure.                                                   */
/******************************************************************************/
typedef struct
{
  MQHCONN hConn;                       // Connection handle.  
  char qmgr[MQ_Q_MGR_NAME_LENGTH + 1]; // Queue manager name.  
  MQLONG cc;                           // Completion code. 
  MQLONG rc;                           // Reason code. 
} mqconn_t;

/* Initialize MQ connection structure.  */
void mqInit       (mqconn_t *conn);

/* Connect to queue manager.  */
int  mqConnect    (mqconn_t *conn);

/* Disconnect from queue manager.  */
int  mqDisconnect (mqconn_t *conn);

/* Get error message for reason code.  */
const char *mqReasonStr (MQLONG reason_code);

/* Open a queue. Pass NULL or "" for qmgr to target the local queue manager. */
int mqOpenQueue  (mqconn_t *conn, const char *qname, const char *qmgr,
                  MQLONG options, MQHOBJ *hObj);

/* Close a queue opened with mqOpenQueue.  */
int mqCloseQueue (mqconn_t *conn, MQHOBJ *hObj);

/* Inquire error log entries from queue manager.  */
int mqInquireErrorLog (mqconn_t *conn);

#endif /* MQUTIL_H */
