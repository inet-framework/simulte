// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes;

import inet.mobility.static.StationaryMobility;
import lte.stack.phy.LteNicRelay;


// 
// Relay Module
//
module Relay
{
    parameters:
        @networkNode;
        @display("i=device/receiverdish;bgb=250,200");

        //# Node specs
        string nodeType = "RELAY";        // DO NOT CHANGE
        int masterId = default(0);
        int macNodeId = default(0);
        int macCellId = default(0);

    gates:
        input radioIn[2] @directIn;     // connection to master

    submodules:
        mobility: StationaryMobility {
            @display("p=50,75;is=s");
        }
        nicUe: LteNicRelay {
            @display("p=100,150");
            LtePdcpRrcType = "LtePdcpRrcRelayUe";
            LteMacType = "LteMacRelayUe";
        }
        nicEnb: LteNicRelay {
            @display("p=200,150");
            LtePdcpRrcType = "LtePdcpRrcRelayEnb";
            LteMacType = "LteMacRelayEnb";
        }

    connections allowunconnected:
        //#
        //# Internal Lte connections
        //#
        nicUe.upperLayerIn <-- nicEnb.upperLayerOut;
        nicUe.upperLayerOut --> nicEnb.upperLayerIn;

        //# 
        //# External connection to air interface
        //#

        radioIn[0] --> nicUe.radioIn;
        radioIn[1] --> nicEnb.radioIn;
}

//
// Multiplexer to select Un (DeNB <--> relay) or Uu (relay <--> UE)
// interface above the NIC.
// Each interface is connected to an LteStack
//
simple UnUuMux
{
    parameters:
        @display("i=block/dispatch");

    gates:
        inout Un;            // to/from ueStack
        inout Uu;            // to/from enbStack
        inout lowerGate;    // to/from NIC
}


