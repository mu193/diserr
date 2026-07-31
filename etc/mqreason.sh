/* mqreason.h - IBM MQ reason code to string mapping.

   This header defines symbolic constants for MQ reason codes and
   declares the conversion function.

   Functions:
     mqrc2str - Return the symbolic name for an MQ reason code
*/

#ifndef MQREASON_H
#define MQREASON_H

const char* mqrc2str( int reason );

