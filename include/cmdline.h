#ifndef CMDLINE_H
#define CMDLINE_H

/* Command line arguments structure.  */
typedef struct
{
  char *qmgr_name;  /* Queue manager name.  */
} cmdline_args_t;

/* Initialize arguments with defaults.  */
void cmdline_init (cmdline_args_t *args);

/* Parse command line arguments.  */
int cmdline_parse (int argc, char *argv[], cmdline_args_t *args);

/* Print usage information.  */
void cmdline_usage (const char *progname);

/* Free allocated memory.  */
void cmdline_cleanup (cmdline_args_t *args);

#endif /* CMDLINE_H */
