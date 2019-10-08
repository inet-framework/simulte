#!/bin/bash

cmd=opp_run
libinet=INET
liblte=lte
nedinet=/home/ich/libs/inet/4.0.0/src
nedlte=/home/ich/libs/simulte/simulte_inet4_compat/src
nedlteexample=/home/ich/libs/simulte/simulte_inet4_compat/simulations
nedinetcompat=/home/ich/libs/simulte/simulte_inet4_compat/compatibility/include/inet4_compat

cmd="$cmd -l $liblte -l $libinet \
    -n $nedinet:$nedlte:$nedlteexample:$nedinetcompat -f omnetpp.ini"
echo $cmd
$cmd
