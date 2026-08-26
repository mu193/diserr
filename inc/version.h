
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
*/

#ifndef VERSION_H
#define VERSION_H

#define VERSION "1.2.1"

void version(void);

#endif /* VERSION_H */
