/* osutil.h - OS-level helper interface.

   This header defines helper entry points for process and pipe utilities.

   Functions:
     osutilMqrcTextToJson - Run mqrc and convert text input to JSON output
*/

#ifndef OSUTIL_H
#define OSUTIL_H

/******************************************************************************/
/*                            P R O T O T Y P E S                             */
/******************************************************************************/
int osutilMqrcTextToJson (const char *text, char **json);

#endif /* OSUTIL_H */
