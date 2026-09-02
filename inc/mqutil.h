/* mqutil.h - IBM MQ utility interface.

   This header defines the MQ connection structure and function prototypes
   for the mqutil module.

   Types:
     mqconn_t - Structure holding MQ connection state

   Functions:
     mqInit             - Initialize MQ connection structure
     mqConnect          - Connect to a queue manager
     mqDisconnect       - Disconnect from a queue manager
     mqOpenQueue        - Open a queue
     mqCloseQueue       - Close a queue opened with mqOpenQueue
      mqPutMsg           - Put a message to an open queue
     mqInquireErrorLog  - Inquire error log entries from queue manager
*/

#ifndef MQUTIL_H
#define MQUTIL_H

#include <stdio.h>
#include <cmqc.h>

/******************************************************************************/
/*                               T Y P E S                                    */
/******************************************************************************/

// ---------------------------------------------------------
// Real stdout stream, saved before -o/--output may redirect
// the "stdout" stream to a file.  --print-time output always
// goes here so timestamps are never sent to the output file.
// ---------------------------------------------------------
extern FILE *gRealStdout;

// ---------------------------------------------------------
// MQ connection state.
// ---------------------------------------------------------
typedef struct
{
  MQHCONN hConn;                       // Connection handle.  
  char qmgr[MQ_Q_MGR_NAME_LENGTH + 1]; // Queue manager name.  
  MQLONG cc;                           // Completion code. 
  MQLONG rc;                           // Reason code. 
} mqconn_t;

/******************************************************************************/
/*                            P R O T O T Y P E S                             */
/******************************************************************************/
void        mqInit             (mqconn_t *conn);
int         mqConnect          (mqconn_t *conn);
int         mqDisconnect       (mqconn_t *conn);
int         mqOpenQueue        (mqconn_t *conn, const char *qName,
                                const char *rQmgr, MQLONG options,
                                MQHOBJ *hObj);
int         mqCloseQueue       (mqconn_t *conn, MQHOBJ *hObj);
int         mqPutMsg           (mqconn_t *conn, MQHOBJ hObj,
                                void *msg, MQLONG msgLen);
int         mqInquireErrorLog  (mqconn_t *conn);

#endif /* MQUTIL_H */
