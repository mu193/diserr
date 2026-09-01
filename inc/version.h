
/* version.h - Version information interface.

  This header defines the application version constant and the function
  prototype for displaying version information.

  Constants:
    VERSION - Application version string

  Functions:
    version  - Display version information

    VERSIONS:
      1.0.0 - Initial release
      1.1.0 - failed functionality 
      1.0.1 - core dump rqmgr bug fix
      1.0.2 - reply queue renamed
      1.2.0 - client -- proxy -- target
      1.2.1 - new CLI --output attribute and output functionality
      1.2.2 - fix --print-time and MQRCCF_NONE_FOUND redirection with --output
      1.2.3 - code cleanup for 1.2
      2.0.0 - automatic collection of logs for remote queue manager
*/

#ifndef VERSION_H
#define VERSION_H

#define VERSION "2.0.0"

void version(void);

#endif /* VERSION_H */
