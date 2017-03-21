#!/bin/bash
chmod +x ipcrm
./ipcrm --all=msg

./server 0 > server.log &
./server 1&
./server 2&

./test $@

./ipcrm --al=msg 

