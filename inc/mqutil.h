#ifndef MQUTIL_H
#define MQUTIL_H

#include <cmqc.h>

/* MQ Connection structure.  */
typedef struct
{
  MQHCONN hConn;           /* Connection handle.  */
  char qmgr_name[MQ_Q_MGR_NAME_LENGTH + 1];
  MQLONG comp_code;        /* Completion code.  */
  MQLONG reason_code;      /* Reason code.  */
} mqconn_t;

/* Initialize MQ connection structure.  */
void mqInit       (mqconn_t *mq_conn, const char *qmgr_name);

/* Connect to queue manager.  */
int  mqConnect    (mqconn_t *mq_conn);

/* Disconnect from queue manager.  */
int  mqDisconnect (mqconn_t *mq_conn);

/* Get error message for reason code.  */
const char *mqReasonStr (MQLONG reason_code);

#endif /* MQUTIL_H */
