// 
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.apps.gaming;

import inet.applications.udpapp.UDPBasicApp;

simple Gaming extends UDPBasicApp
{
    parameters:
        localPort = default(2000);
        destPort = default(3000);
        int a1 @unit(B)=default(120B);
		int b1 @unit(B)=default(36B);
		int a2 @unit(s)=default(55s);
		int b2 @unit(s) =default(6s);
		volatile double formula1=log(-log(uniform(0,1)));
		volatile double formula2=log(-log(uniform(0,1)));
		double freq @unit(s)=replaceUnit(formula1,"s");
		sendInterval =replaceUnit((a2-b2*log(-log(uniform(0,1)))),"ms");
		double size @unit(B)=replaceUnit(formula2,"B");
        messageLength =replaceUnit((a1-b1*log(-log(uniform(0,1)))),"B");
        @display("i=block/source");
        @class(Gaming);
}
