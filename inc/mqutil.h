/* mqutil.h - IBM MQ utility interface.

   This header defines the MQ connection structure and function prototypes
   for the mqutil module.

   Types:
     mqconn_t - Structure holding MQ connection state

   Functions:
     mqInit             - Initialize MQ connection structure
     mqConnect          - Connect to a queue manager
     mqDisconnect       - Disconnect from a queue manager
     mqReasonStr        - Get error message for a reason code
     mqOpenQueue        - Open a queue
     mqCloseQueue       - Close a queue opened with mqOpenQueue
     mqInquireErrorLog  - Inquire error log entries from queue manager
*/

#ifndef MQUTIL_H
#define MQUTIL_H

#include <cmqc.h>

/******************************************************************************/
/*                               T Y P E S                                    */
/******************************************************************************/

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
const char *mqReasonStr        (MQLONG reason_code);
int         mqOpenQueue        (mqconn_t *conn, const char *qName,
                                const char *rQmgr, MQLONG options,
                                MQHOBJ *hObj);
int         mqCloseQueue       (mqconn_t *conn, MQHOBJ *hObj);
int         mqInquireErrorLog  (mqconn_t *conn);

#endif /* MQUTIL_H */
