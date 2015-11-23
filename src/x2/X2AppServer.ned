// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 

package lte.x2;

import inet.applications.sctpapp.SCTPServer;

// X2AppServer
//
// This module extends the SCTPServer application, by adding
// an input gate to receive messages from the X2 Manager module
//
simple X2AppServer extends SCTPServer
{
    parameters:
        @class("X2AppServer");

    gates:    
        input x2ManagerIn @labels(X2Msg);       
}

