# mq.diserr

`mq.diserr` is an IBM MQ utility for retrieving and displaying error log entries from local or remote queue managers.

## Usage

```text
Usage: diserr [OPTIONS]

Options:
  -m, --qmgr        <name>   Connect in binding mode to this queue manager
  -c, --client      <name>   Connect in client mode to this queue manager
  -r, --remQmgr     <name>   Execute PCF on this remote queue manager
  -l, --logFile     <name>   Error log file name (e.g. AMQERR01)
  -t, --timeStamp   <iso>    Show entries after ISO 8601 timeStamp
  -f, --format      <fmt>    Output format: text (default) or json
  -v, --verbose              Print all PCF selectors (default: log records only)
      --print-time           Print timestamp with each log entry
      --version              Print version information
  -h, --help                 Display this help message

Note: -m and -c are mutually exclusive.

Timestamp format (-t):
  Time components may be omitted from the right; missing parts default to 0.
  The timezone suffix controls how the time is interpreted:

    No suffix     Local system time, automatically converted to UTC
                  e.g.  -t 2026-07-27T22:10

    Z suffix      Explicit UTC (Zulu)
                  e.g.  -t 2026-07-27T22:10:11.226Z

    +HH:MM        Explicit UTC east offset (e.g. CET=+01:00, CEST=+02:00)
                  e.g.  -t 2026-07-27T22:10+02:00

    -HH:MM        Explicit UTC west offset (e.g. EST=-05:00)
                  e.g.  -t 2026-07-27T22:10-05:00

  In all cases the timeStamp is normalised to yyyy-mm-ddThh:mm:ss.sssZ
  before being sent to the queue manager command server.
```

## Versions

| Version | Description |
| --- | --- |
| 1.0.0 | Initial release |
| 1.0.1 | Core dump remote queue-manager bug fix |

