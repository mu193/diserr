/* cmdline.h - Command line argument parsing interface.

   This header defines the argument structure and function prototypes
   for the cmdline module.

   Types:
     cmdline_t - Structure holding parsed command line arguments

   Functions:
     cmdlInit     - Initialize arguments structure with defaults
     cmdlParse    - Parse command line arguments from argc/argv
     cmdlUsage    - Display usage information to stdout
     cmdlCleanup  - Free allocated memory in arguments structure
*/

#ifndef CMDLINE_H
#define CMDLINE_H

/******************************************************************************/
/*                               T Y P E S                                    */
/******************************************************************************/

// ---------------------------------------------------------
// Connection mode selected by -m (binding) or -c (client). 
// ---------------------------------------------------------
typedef enum
{
  BINDING = 1,   // local binding via shared memory (default) 
  CLIENT  = 2    // TCP client via MQ channel infrastructure  
} connType_t;

// ---------------------------------------------------------
// Parsed command line arguments. 
// ---------------------------------------------------------
typedef struct
{
  char       *qmgr;       // queue manager name: binding (-m) or client (-c)
  connType_t  connType;   // connection mode: BINDING (-m) / CLIENT (-c)
  char       *logFile;    // error log file name (-l)
  char       *timeStamp;  // ISO 8601 timestamp (-t) for filtering log entries
  char       *remQmgr;    // remote queue manager name (-r) for PCF execution
  int         verbose;    // verbose output (-v): print all selectors
} cmdline_t;

extern cmdline_t args;

/******************************************************************************/
/*                            P R O T O T Y P E S                             */
/******************************************************************************/
void cmdlInit    (void);
void cmdlUsage   (const char *progname);
int  cmdlParse   (int argc, char *argv[]);
void cmdlCleanup (void);

#endif /* CMDLINE_H */
