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
} mq_connection_t;

/* Initialize MQ connection structure.  */
void mq_init (mq_connection_t *mq_conn, const char *qmgr_name);

/* Connect to queue manager.  */
int mq_connect (mq_connection_t *mq_conn);

/* Disconnect from queue manager.  */
int mq_disconnect (mq_connection_t *mq_conn);

/* Get error message for reason code.  */
const char *mq_reason_str (MQLONG reason_code);

#endif /* MQUTIL_H */
