/* mqconvert.h - IBM MQ constant to string conversion interface.

   Functions:
     mqselector2str - Return the symbolic name for an MQIASY_* / MQCASY_* selector,
                      or NULL for unknown selectors
     mqcft2str      - Return the symbolic name for an MQCFT_* PCF structure type
*/

#ifndef MQCONVERT_H
#define MQCONVERT_H

const char *mqselector2str (int selector);
const char *mqcft2str      (int type);

#endif /* MQCONVERT_H */
