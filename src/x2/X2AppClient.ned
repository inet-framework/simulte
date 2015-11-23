// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.x2;

import inet.applications.sctpapp.SCTPClient;

// X2AppClient
//
// This module extends the SCTPClient application, by adding
// an output gate to send messages to the X2 Manager
//
simple X2AppClient extends SCTPClient
{
    parameters:
        @class("X2AppClient");
        connectPort = default(5000); // it will be overwritten by the initialization procedure
		startTime = default(0s);
    gates:
        output x2ManagerOut @labels(X2Msg);     
}

