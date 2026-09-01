/* mqlogctl.h - IBM MQ error log control interface.

	 This header declares functions for controlling and retrieving
	 queue-manager error log files.

	 Types:
		 mqlogctl_t - Structure holding log-control request data

	 Functions:
		 None yet
*/

#ifndef MQLOGCTL_H
#define MQLOGCTL_H

#include <cmqc.h>

/******************************************************************************/
/*                               D E F I N E S                                */
/******************************************************************************/
#define MQLOGCTL_ISO_TIME_BUFLEN 25 // yyyy-mm-ddThh:mm:ss.sssZ + null terminator

/******************************************************************************/
/*                               T Y P E S                                    */
/******************************************************************************/

// ---------------------------------------------------------
// Log-control request data.
// ---------------------------------------------------------
typedef struct
{
  char qmgr[MQ_Q_MGR_NAME_LENGTH];           // Queue manager name.
  char timeStamp[MQLOGCTL_ISO_TIME_BUFLEN];  // Normalized ISO timestamp.
} mqlogctl_t;

#endif /* MQLOGCTL_H */
