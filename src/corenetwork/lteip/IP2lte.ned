//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
package lte.corenetwork.lteip;

//
// IP2lte module performs as a bridge between the network layer and the LTE protocol stack
// It has a slightly different behavior in the UE rather than in the eNB
// It basically:
// - Registers itself as an interface for being accessible from upper layers (e.g. IP Layer)
// - In the downstream direction adds control information to IP packets (5-tuple). Moreover in the ENb 
//   module it adds the destination id to the control information, in order to send packets to the correct UE  
//
simple IP2lte
{
    parameters:
        string nodeType;    
        string interfaceTableModule;
        string routingTableModule;
        @display("i=block/layer");
    gates:
        // connection to network layer.
        // These gate names are recognized by the network layer (same as the ones in the INic interface) 
        input upperLayerIn;
        output upperLayerOut;

        // connection to the lte stack
        inout stackLte @labels(IPDatagram);
}
