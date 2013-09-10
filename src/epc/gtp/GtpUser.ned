//
//                           SimuLTE
// Copyright (C) 2013 Antonio Virdis
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.epc.gtp;

//
// Implements the GTP-U protocol
//
simple GtpUser
{
    parameters:
        int localPort = default(31);

        int tunnelPeerPort = default(31);
        string teidFileName;
        string tftFileName;

        bool filter = default(true);

        @display("i=block/tunnel");

    gates:
        output udpOut;
        input udpIn;

        input trafficFlowFilterGate;
        output pppGate;
}

