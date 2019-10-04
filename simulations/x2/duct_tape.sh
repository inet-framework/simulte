#!/bin/bash

cmd=opp_run
libinet=INET
liblte=lte
nedinet=/home/ich/libs/inet/4.0.0/src/inet
nedlte=/home/ich/libs/simulte/simulte_inet4_compat/src
nedlteexample=/home/ich/libs/simulte/simulte_inet4_compat/simulations

cmd="$cmd -l $libinet -l $liblte -n $nedinet:$nedlte:$nedlteexample -f omnetpp.ini"
echo $cmd
$cmd
