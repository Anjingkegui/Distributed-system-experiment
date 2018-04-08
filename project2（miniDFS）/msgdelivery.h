#ifndef _MSGDELIVERY_H
#define _MSGDELIVERY_H

#include "socket.h"
#include "message.pb.h"

int  readmsg(Socket &sock, msgcontainer &msg);
int  writemsg(Socket &sock, msgcontainer &msg);

#endif
