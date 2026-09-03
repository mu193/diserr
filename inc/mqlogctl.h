/* mqlogctl.h - IBM MQ error log control interface.

   This header declares functions for controlling and retrieving
   queue-manager error log files.

   Types:
     mqlogctl_t - Structure holding log-control request data

   Functions:
     addQmgr    - register a queue manager in the daemon configuration
     listQmgr   - list queue-manager entries from daemon configuration
     foreground - run in foreground mode
     daemonize  - run in daemon mode
*/

#ifndef MQLOGCTL_H
#define MQLOGCTL_H

#include <time.h>
#include <cmqc.h>
#include "mqutil.h"

/******************************************************************************/
/*                               D E F I N E S                                */
/******************************************************************************/

// ---------------------------------------------------------
// MQ error log control Queue name	& properties
// ---------------------------------------------------------
#define MQ_DISERR_CTRL_QUEUE       "MONITOR.DISERR.CONTROL"
#define MQ_DISERR_CTRL_MSG_EXPIRY   30  // Message expiry in days

// ---------------------------------------------------------
// MQ log item lengths
// ---------------------------------------------------------
#define MQLOGCTL_ISO_TIME_LEN 24     // yyyy-mm-ddThh:mm:ss.sssZ; no null terminator
#define MQLOGCTL_LOG_FILENAME_LEN 8  // AMQERR01, AMQERR02, or AMQERR03; no null terminator

// ---------------------------------------------------------
// MQ log control structure identifiers and defaults
// ---------------------------------------------------------
#define MQLOGCTL_STRUC_ID "MQLC"
#define MQLOGCTL_VERSION_1 1
#define MQLOGCTL_FILE_NAME "AMQERR01"

#define MQLOGCTL_DEFAULT { .structId = {'M','Q','L','C'}, \
                         .version = MQLOGCTL_VERSION_1, \
                         .collectionStop = 0, \
                         .collectionStart = 0, \
                         .retryCount = 0, \
                         .qmgr = {0}, \
                         .lastLogEntry = {0}, \
                         .fileName = {'A','M','Q','E','R','R','0','1'} }

/******************************************************************************/
/*                               T Y P E S                                    */
/******************************************************************************/

// ---------------------------------------------------------
// Log-control request data.
// ---------------------------------------------------------
typedef struct
{
  char structId[4];                         // Structure identifier.
  unsigned short version;                   // Structure version.
  time_t collectionStop;                    // Timestamp when log collection is paused.
  time_t collectionStart;                   // Timestamp when log collection resumes.
  unsigned short retryCount;                // Number of retry attempts for log collection.
  char qmgr[MQ_Q_MGR_NAME_LENGTH];          // Queue manager name.
  char lastLogEntry[MQLOGCTL_ISO_TIME_LEN]; // Last log entry's normalized ISO timestamp.
  char fileName[MQLOGCTL_LOG_FILENAME_LEN]; // Error log filename.
} mqlogctl_t;


/******************************************************************************/
/* prototypes */
/******************************************************************************/

int addQmgr (mqconn_t *conn);
int listQmgr (mqconn_t *conn);
int foreground (mqconn_t *conn);
int daemonize  (mqconn_t *conn);

#endif /* MQLOGCTL_H */
