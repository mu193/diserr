/* cmdline.c - Command line argument parsing.

   This module handles parsing and validation of command line arguments
   for the diserr utility. It uses getopt_long to support both short
   and long option formats.

   Functions:
     cmdlInit              - Initialize arguments structure with defaults
     cmdlParse             - Parse command line arguments from argc/argv
     cmdlUsage             - Display usage information to stdout
     cmdlCleanup           - Free allocated memory in arguments structure
     cmdlValidateQmgrName  - Validate a queue manager name argument (static)
     cmdlNormalizeIsoTime  - Validate and normalize ISO 8601 timestamp to
                             yyyy-mm-ddThh:mm:ss.sssZ (static)
*/


/******************************************************************************/
/*                                D E F I N E S                               */
/******************************************************************************/

// -------------------------------------
// compiler features 
// -------------------------------------
#define _GNU_SOURCE              // GNU extensions (timegm) + POSIX.1-2008

// -------------------------------------
// Final output is fixed at 24 chars + null; 
// fullTs may be longer before UTC normalization.
// -------------------------------------
#define CMDL_ISO_TIME_BUFLEN 25  // yyyy-mm-ddThh:mm:ss.sssZ + null terminator
#define CMDL_FULL_TS_BUFLEN  32  // yyyy-mm-ddT + accepted time-only input

/******************************************************************************/
/*                               I N C L U D E S                              */
/******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <getopt.h>
#include <cmqc.h>
#include "cmdline.h"

/******************************************************************************/
/*                          G L O B A L   V A R S                             */
/******************************************************************************/
cmdline_t args;

/******************************************************************************/
/*                                                                            */
/*                             F U N C T I O N S                              */
/*                                                                            */
/******************************************************************************/

static int cmdlNormalizeIsoTime (const char *ts, char *buf, size_t bufLen);
static int cmdlValidateQmgrName (const char *value, const char *optName);

/******************************************************************************/
/* Initialize command line arguments structure with default values.           */
/******************************************************************************/
void cmdlInit (void)
{
  args.qmgr      = NULL;      // qmgr name (-m/-c)
  args.connType  = BINDING;   // connection type: binding (-m) or client (-c)
  args.logFile   = NULL;      // error log file name (-l)
  args.timeStamp = NULL;      // ISO 8601 timestamp for filtering (-t)
  args.remQmgr   = NULL;      // remote qmgr name for PCF execution (-r)
  args.nodeQmgr  = NULL;      // qmgr name to add to daemon configuration
  args.config    = 0;         // daemon configuration enabled (--config)
  args.list      = 0;         // list daemon configuration entries (--list)
  args.output    = strdup ("stdout"); // output destination (-o)
  args.daemon    = 0;         // daemon mode (-d)
  args.verbose   = 0;         // print all selectors (-v)
  args.format    = FMT_TEXT;  // output format: text or json (-f)
  args.printTime = 0;         // print timestamp with each entry (--print-time)
  args.version   = 0;         // print version information (--version)
}

/******************************************************************************/
/* Parse command line arguments.                                              */
/*                                                                            */
/* retrun code:                                                               */
/*   0:  on success,                                                          */
/*   1:  for help,                                                            */
/*   2:  for version,                                                         */
/*  -1:  on parse error.                                                      */
/******************************************************************************/
int cmdlParse (int argc, char *argv[])
{
  int opt;
  int option_index = 0;

  static struct option long_options[] = {
    {"qmgr",       required_argument, 0, 'm'},
    {"client",     required_argument, 0, 'c'},
    {"remQmgr",     required_argument, 0, 'r'},
    {"logFile",     required_argument, 0, 'l'},
    {"timeStamp",   required_argument, 0, 't'},
    {"format",      required_argument, 0, 'f'},
    {"output",      required_argument, 0, 'o'},
    {"daemon",      no_argument,       0, 'd'},
    {"config",      no_argument,       0, 1100},
    {"add-qmgr",    required_argument, 0, 1101},
    {"list",        no_argument,       0, 1102},
    {"help",        no_argument,       0, 'h'},
    {"verbose",     no_argument,       0, 'v'},
    {"print-time",  no_argument,       0, 1001},
    {"version",     no_argument,       0, 1002},
    {0, 0, 0, 0}
  };

  // -------------------------------------------------------
  // Parse each command-line option and update args accordingly.
  // -------------------------------------------------------
  while ((opt = getopt_long (argc, argv, "m:c:r:l:t:f:o:dhv",
                             long_options, &option_index)) != -1)
  {
    switch (opt)
    {
      // ---------------------------------------------------
      // Connection mode: binding (-m) 
      //   sets args.qmgr and args.connType to BINDING
      // ---------------------------------------------------
      case 'm':
        if (args.qmgr != NULL)    // (-c) was already specified, 
        {                         //   error: mutually exclusive
          fprintf (stderr, "Error: -m and -c are mutually exclusive\n"); 
          goto _door_error; 
        }
        if (cmdlValidateQmgrName (optarg, "--qmgr") != 0)
          goto _door_error;
        args.qmgr     = strdup (optarg);
        args.connType = BINDING;
        if (args.qmgr == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      // ---------------------------------------------------
      // Connection mode: client (-c)
      //   sets args.qmgr and args.connType to CLIENT
      // ---------------------------------------------------
      case 'c':
        if (args.qmgr != NULL)    // (-m) was already specified, 
        {                         //   error: mutually exclusive
          fprintf (stderr, "Error: -m and -c are mutually exclusive\n");
          goto _door_error;
        }
        if (cmdlValidateQmgrName (optarg, "--client") != 0)
          goto _door_error;
        args.qmgr     = strdup (optarg);
        args.connType = CLIENT;
        if (args.qmgr == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      // ---------------------------------------------------
      // Remote queue manager for PCF execution (-r)
      //   sets args.remQmgr
      // ---------------------------------------------------
      case 'r':
        if (cmdlValidateQmgrName (optarg, "--remQmgr") != 0)
          goto _door_error;
        args.remQmgr = strdup (optarg);
        if (args.remQmgr == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      // ---------------------------------------------------
      // Log file (-l)
      //   sets args.logFile
      // ---------------------------------------------------
      case 'l':   // only allow AMQERR01, AMQERR02, AMQERR03, 
        {         //   with optional .LOG suffix
          if (!(   strncmp (optarg, "AMQERR0", 7) == 0
               && (optarg[7] >= '1' && optarg[7] <= '3')
               && (optarg[8] == '\0' || strcmp (optarg + 8, ".LOG") == 0)))
          {
            fprintf (stderr, "Error: log file must be AMQERR01/2/3[.LOG]\n");
            goto _door_error;
          }
        }
        args.logFile = strdup (optarg);
        if (args.logFile == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      // ---------------------------------------------------
      // Timestamp filter (-t)
      //   validates and normalizes to yyyy-mm-ddThh:mm:ss.sssZ,
      //   sets args.timeStamp
      // ---------------------------------------------------
      case 't':
        {
          char isoBuf[CMDL_ISO_TIME_BUFLEN];
          if (cmdlNormalizeIsoTime (optarg, isoBuf, sizeof (isoBuf)) != 0)
            goto _door_error;
          args.timeStamp = strdup (isoBuf);
          if (args.timeStamp == NULL)
          {
            fprintf (stderr, "Error: Memory allocation failed\n");
            goto _door_error;
          }
        }
        break;

      // ---------------------------------------------------
      // Output format (-f)
      //   accepts text or json
      // ---------------------------------------------------
      case 'f':
        if (strcmp (optarg, "text") == 0)
          args.format = FMT_TEXT;
        else if (strcmp (optarg, "json") == 0)
          args.format = FMT_JSON;
        else
        {
          fprintf (stderr, "Error: -f/--format must be 'text' or 'json'\n");
          goto _door_error;
        }
        break;

      // ---------------------------------------------------
      // Output destination (-o)
      //   file name or stdout, stdout is default
      // ---------------------------------------------------
      case 'o':
        free (args.output);
        args.output = strdup (optarg);
        if (args.output == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      // ---------------------------------------------------
      // Daemon mode (-d)
      //   collect data from all configured remote queue managers
      // ---------------------------------------------------
      case 'd':
        args.daemon = 1;
        break;

      // ---------------------------------------------------
      // Configuration flag (--config)
      // ---------------------------------------------------
      case 1100:
        args.config = 1;
        break;

      // ---------------------------------------------------
      // Add queue manager to daemon configuration (--add-qmgr)
      // ---------------------------------------------------
      case 1101:
        if (!args.config)
        {
          fprintf (stderr, "Error: --add-qmgr requires --config\n");
          goto _door_error;
        }
        if (cmdlValidateQmgrName (optarg, "--add-qmgr") != 0)
          goto _door_error;
        args.nodeQmgr = strdup (optarg);
        if (args.nodeQmgr == NULL)
        {
          fprintf (stderr, "Error: Memory allocation failed\n");
          goto _door_error;
        }
        break;

      // ---------------------------------------------------
      // List daemon configuration entries (--list)
      // ---------------------------------------------------
      case 1102:
        if (!args.config)
        {
          fprintf (stderr, "Error: --list requires --config\n");
          goto _door_error;
        }
        args.list = 1;
        break;

      // ---------------------------------------------------
      // Verbose mode (-v)
      // ---------------------------------------------------
      case 'v':
        args.verbose = 1;
        break;

      // ---------------------------------------------------
      // Print time (--print-time), long format only.
      //   print out the timestamp of the last log entry
      //   in format yyyy-mm-ddThh:mm:ss.sssZ 
      // ---------------------------------------------------
      case 1001:
        args.printTime = 1;
        break;

      // ---------------------------------------------------
      // Version (--version)
      // ---------------------------------------------------
      case 1002:
        args.version = 1;
        break;

      // ---------------------------------------------------
      // Help (-h) / usage
      // ---------------------------------------------------
      case '?':
        goto _door_error;

      // ---------------------------------------------------
      // Help (-h) / usage
      // ---------------------------------------------------
      case 'h':
        cmdlUsage (argv[0]);
        return 1;

      default:
        goto _door_error;
    }
  }

  if (args.version)
  {
    if (argc != 2 || strcmp (argv[1], "--version") != 0)
    {
      fprintf (stderr, "Error: --version cannot be combined with other arguments\n");
      goto _door_error;
    }
    return 2;
  }

//_door_ok:
  return 0;

  _door_error:
    return -1;
}

/******************************************************************************/
/* Display usage information.                                                 */
/******************************************************************************/
void cmdlUsage (const char *progname)
{
  printf ("Usage: %s [OPTIONS]\n", progname);
  printf ("\nOptions:\n");
  printf ("  -m, --qmgr        <name>   Connect in binding mode to this queue manager\n");
  printf ("  -c, --client      <name>   Connect in client mode to this queue manager\n");
  printf ("  -r, --remQmgr     <name>   Execute PCF on this remote queue manager\n");
  printf ("  -l, --logFile     <name>   Error log file name (e.g. AMQERR01)\n");
  printf ("  -t, --timeStamp   <iso>    Show entries after this ISO 8601 timeStamp\n");
  printf ("  -f, --format      <fmt>    Output format: text (default) or json\n");
  printf ("  -o, --output      <dest>   Output destination: stdout or a file name\n");
  printf ("  -d, --daemon               Collect data from all configured remote queue managers\n");
  printf ("      --config                Enable daemon configuration mode\n");
  printf ("      --add-qmgr     <name>   Add queue manager to daemon configuration (requires --config)\n");
  printf ("      --list                  List queue managers from daemon configuration (requires --config)\n");
  printf ("  -v, --verbose              Print all PCF selectors (default: log records only)\n");
  printf ("      --print-time           Print timestamp with each log entry\n");
  printf ("      --version              Print version information\n");
  printf ("  -h, --help                 Display this help message\n");
  printf ("\nNote: -m and -c are mutually exclusive.\n");
  printf ("\nTimestamp format (-t):\n");
  printf ("  Time components may be omitted from the right; missing parts default to 0.\n");
  printf ("  The timezone suffix controls how the time is interpreted:\n");
  printf ("\n");
  printf ("    No suffix     Local system time, automatically converted to UTC\n");
  printf ("                  e.g.  -t 2026-07-27T22:10\n");
  printf ("\n");
  printf ("    Z suffix      Explicit UTC (Zulu)\n");
  printf ("                  e.g.  -t 2026-07-27T22:10:11.226Z\n");
  printf ("\n");
  printf ("    +HH:MM        Explicit UTC east offset (e.g. CET=+01:00, CEST=+02:00)\n");
  printf ("                  e.g.  -t 2026-07-27T22:10+02:00\n");
  printf ("\n");
  printf ("    -HH:MM        Explicit UTC west offset (e.g. EST=-05:00)\n");
  printf ("                  e.g.  -t 2026-07-27T22:10-05:00\n");
  printf ("\n");
  printf ("  In all cases the timeStamp is normalised to yyyy-mm-ddThh:mm:ss.sssZ\n");
  printf ("  before being sent to the queue manager command server.\n");
}

/******************************************************************************/
/* Free allocated memory in command line arguments structure.                 */
/******************************************************************************/
void cmdlCleanup (void)
{
  if (args.qmgr != NULL)
  {
    free (args.qmgr);
    args.qmgr = NULL;
  }

  if (args.logFile != NULL)
  {
    free (args.logFile);
    args.logFile = NULL;
  }

  if (args.timeStamp != NULL)
  {
    free (args.timeStamp);
    args.timeStamp = NULL;
  }

  if (args.remQmgr != NULL)
  {
    free (args.remQmgr);
    args.remQmgr = NULL;
  }

  if (args.nodeQmgr != NULL)
  {
    free (args.nodeQmgr);
    args.nodeQmgr = NULL;
  }


  if (args.output != NULL)
  {
    free (args.output);
    args.output = NULL;
  }
}

/******************************************************************************/
/* Validate queue-manager name length against MQ fixed-size fields.           */
/******************************************************************************/
static int cmdlValidateQmgrName (const char *value, const char *optName)
{
  if (strlen (value) > MQ_Q_MGR_NAME_LENGTH)
  {
    fprintf (stderr,
             "Error: %s value exceeds MQ_Q_MGR_NAME_LENGTH (%d)\n",
             optName, MQ_Q_MGR_NAME_LENGTH);
    return -1;
  }
  return 0;
}

/******************************************************************************/
/* Validate and normalize a partial ISO 8601 timestamp to the full 24-char    */
/* MQ format yyyy-mm-ddThh:mm:ss.sssZ.                                        */
/*                                                                            */
/* Input forms accepted:                                                      */
/*   YYYY-MM-DD[Thh[:mm[:ss[.sss]]]][Z|+HH:MM|-HH:MM]  full or partial date   */
/*   HH:MM or HH:MM:SS                                  time-only: today      */
/*                                                                            */
/* Examples:                                                                  */
/*   13:15                 -> today from 13:15:00 (local)                     */
/*   13:15:30              -> today from 13:15:30 (local)                     */
/*   2026-05-31            -> from 2026-05-31T00:00:00 on (local)             */
/*   2026-05-31T13:15      -> from 2026-05-31T13:15:00 on (local)             */
/*   2026-05-31T13:15Z     -> from 2026-05-31T13:15:00 on (UTC)               */
/*   2026-05-31T13:15+02:00 -> from 2026-05-31T11:15:00Z on (CEST offset)     */
/*                                                                            */
/* Priority 1: fill missing time components (hh, mm, ss, sss) with zero.      */
/* Priority 2: return -1 and print an error if the input cannot be parsed.    */
/*                                                                            */
/* Timezone handling:                                                         */
/*   No suffix         -> local system time, converted to UTC via mktime      */
/*   Z suffix          -> already UTC, used as-is via timegm                  */
/*   +HH:MM / -HH:MM   -> explicit offset, converted to UTC                   */
/*                                                                            */
/* Returns 0 on success, -1 on failure.                                       */
/******************************************************************************/
static int cmdlNormalizeIsoTime(const char *ts    , // input timestamp string
                                char       *buf   , // output buffer for normalized timestamp
                                size_t      bufLen) // length of the output buffer
{
  int         year = 0, mon = 0, day = 0;
  int         hour = 0, min = 0, sec = 0, ms = 0;
  size_t      len  = strlen (ts);
  int         n;
  char        fullTs[CMDL_FULL_TS_BUFLEN];
  const char *origTs = ts;    // keep original for error messages

  // -------------------------------------------------------
  // Time-only input: HH:MM or HH:MM:SS                    
  //   -> prepend today's local date so the rest of the     
  //      function treats it as a local-time timestamp.     
  // -------------------------------------------------------
  if (len >= 5 && ts[2] == ':')
  {
    time_t     now = time (NULL);
    struct tm *lt  = localtime (&now);
    snprintf (fullTs, sizeof (fullTs), "%04d-%02d-%02dT%s",
              lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday, ts);
    ts  = fullTs;
    len = strlen (ts);
  }

  // -------------------------------------------------------
  // At this point ts always has a date prefix (either from  
  // the original input or prepended above). Validate it.   
  // sscanf returns the number of fields successfully parsed.
  // -------------------------------------------------------
  n = sscanf (ts, "%4d-%2d-%2d", &year, &mon, &day);
  if (n < 3 || year < 1 || mon < 1 || mon > 12 || day < 1 || day > 31)
  {
    fprintf (stderr,
             "Error: -t: invalid timestamp '%s'\n"
             "       expected: YYYY-MM-DD[Thh[:mm[:ss[.sss]]]][Z|+HH:MM|-HH:MM]\n",
             origTs);
    return -1;
  }

  // -------------------------------------------------------
  // If a 'T' follows the date, parse optional time fields.  
  // -------------------------------------------------------
  if (len > 10 && ts[10] == 'T')
  {
    sscanf (ts + 11, "%2d:%2d:%2d", &hour, &min, &sec);
    if (hour > 23 || min > 59 || sec > 59)
    {
      fprintf (stderr, "Error: -t: invalid time in timestamp: %s\n", origTs);
      return -1;
    }
  }

  // -------------------------------------------------------
  // Locate timezone indicator (Z / + / -) after position 10 
  // -------------------------------------------------------
  const char *tz = NULL;
  for (size_t i = 11; i < len; i++)
  {
    if (ts[i] == 'Z' || ts[i] == '+' || ts[i] == '-')
    {
      tz = ts + i;
      break;
    }
  }

  // -------------------------------------------------------
  // Parse fractional milliseconds between '.' and tz indicator 
  // -------------------------------------------------------
  if (len >= 20 && ts[19] == '.')
  {
    const char *msEnd   = (tz != NULL) ? tz : ts + len;
    const char *msStart = ts + 20;
    int   ndigits = (int)(msEnd - msStart);
    char  msbuf[4] = "000";
    if (ndigits < 0) ndigits = 0;
    if (ndigits > 3) ndigits = 3;
    for (int i = 0; i < ndigits; i++) msbuf[i] = msStart[i];
    ms = atoi (msbuf);
  }

  // -------------------------------------------------------
  // Build tm struct from parsed components.                 
  // -------------------------------------------------------
  struct tm tm = {0};
  tm.tm_year = year - 1900;
  tm.tm_mon  = mon  - 1;
  tm.tm_mday = day;
  tm.tm_hour = hour;
  tm.tm_min  = min;
  tm.tm_sec  = sec;

  time_t t;
  if (tz == NULL)             // -------------------------------
  {                           //
    tm.tm_isdst = -1;         // No timezone: interpret as local time;
    t = mktime (&tm);         // mktime converts to UTC epoch
  }                           //
  else if (*tz == 'Z')        // ---------------
  {                           //
    t = timegm (&tm);         // Explicit UTC
  }                           //
  else                        // ---------------
  {                                           //
    int tzh = 0, tzm = 0;                     // Explicit offset +HH:MM or -HH:MM
    sscanf (tz + 1, "%2d:%2d", &tzh, &tzm);   //
    int offsetSec = tzh * 3600 + tzm * 60;    // East of UTC (+) means local is
    t = timegm (&tm);                         // ahead: subtract offset for UTC
    t += (*tz == '+') ? -offsetSec : +offsetSec;
  }                                           // ---------------

  struct tm *utc = gmtime (&t);
  if (utc == NULL || bufLen < CMDL_ISO_TIME_BUFLEN
      || strftime (buf, bufLen, "%Y-%m-%dT%H:%M:%S", utc) != 19)
    return -1;
  snprintf (buf + 19, bufLen - 19, ".%03dZ", ms);
  return 0;
}
