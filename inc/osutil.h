/* osutil.h - OS-level helper interface.

   This header defines helper entry points for process and pipe utilities.

   Functions:
     amq2json - Run mqrc and convert text input to JSON output
*/

#ifndef OSUTIL_H
#define OSUTIL_H

/******************************************************************************/
/*                            P R O T O T Y P E S                             */
/******************************************************************************/
int amq2json (const char *text, char **json);

#endif /* OSUTIL_H */
