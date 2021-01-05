//
//                           SimuLTE
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
        string interfaceTableModule; // path to the InterfaceTable module
        string ipOutInterface = default("");    // optional: manual specification of outgoing interface for received IP
        int localPort = default(31);

        int tunnelPeerPort = default(31);
        string teidFileName;
        string tftFileName;

        bool filter = default(true);

        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        input trafficFlowFilterGate;
        output pppGate;
}

//
// Implements a simplified version of the GTP-U protocol
//
simple GtpUserSimplified
{
    parameters:
        string interfaceTableModule;            // path to the InterfaceTable module
        string ipOutInterface = default("");    // optional: manual specification of outgoing interface for received IP 
        int localPort = default(31);
        int tunnelPeerPort = default(31);
        bool filter = default(true);
        
        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        input trafficFlowFilterGate;
        output pppGate;
}

//
// Implements a simplified version of the GTP-U protocol for tunneling packets over X2 (for handover)
//
simple GtpUserX2
{
    parameters:
        int localPort = default(32);
        int tunnelPeerPort = default(32);
        
        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        output lteStackOut;
        input lteStackIn;
}
