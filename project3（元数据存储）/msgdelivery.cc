#include "msgdelivery.h" 
#include <iostream>

int writemsg(Socket &sk, msgcontainer &msg)
{
   char data[512] = {0};
   int datasize = 0;

   sprintf(data, "%04d", msg.ByteSize());
   if (sk.write(data, 4) == -1)
       return -1;

    msg.SerializeToArray(data, sizeof(data));
    if (sk.write(data, msg.ByteSize()) == -1)
        return -1;

   return 0;
}


int readmsg(Socket &sk, msgcontainer &msg)
{
   char data[512] = {0};
   int datasize = 0;

   if (sk.read(data, 4) == -1)
       return -1;

   datasize = atoi(data);

   if (sk.read(data, datasize) < datasize) {
       std::cout << __FILE__ << "read data error " << std::endl;
       exit(0);
       return -1;
   }

   msg.ParseFromArray(data, datasize);
   
   return 0;
}
