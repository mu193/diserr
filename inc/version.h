
/* version.h - Version information interface.

  This header defines the application version constant and the function
  prototype for displaying version information.

  Constants:
    VERSION - Application version string

  Functions:
    version  - Display version information

    VERSIONS:
      1.0.0 - Initial release
      1.0.1 - core dump rqmgr bug fix
  
*/

#ifndef VERSION_H
#define VERSION_H

#define VERSION "1.0.1"

void version(void);

#endif /* VERSION_H */
