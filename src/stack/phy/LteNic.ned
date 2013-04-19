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


package lte.stack.phy;

import inet.linklayer.IWirelessNic;
import lte.stack.mac.LteMac;
import lte.stack.pdcp_rrc.LtePdcpRrc;
import lte.stack.phy.feedback.LteDlFeedbackGenerator;
import lte.stack.rlc.LteRlc;


// 
// Interface for the LTE Stack.
//
moduleinterface LteNic extends IWirelessNic
{
}

// 
// Base module for the LTE Stack.
//
// TODO: implement IWirelessNic
module LteNicBase like LteNic
{
    parameters:
        @display("i=block/ifcard;bgb=250,400");
        string LtePdcpRrcType;      // One of: "LtePdcpRrcUe", "LtePdcpRrcEnb", "LtePdcpRrcRelayUe", "LtePdcpRrcRelayEnb"
        string LteMacType;          // One of: "LteMacUe", "LteMacEnb", "LteMacRelayUe", "LteMacRelayEnb"
        string LtePhyType;

    gates:
        //#
        //# Gates connecting UE/eNB and LTE Stack
        //#
        //# Control Ports
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // to receive messages sent using sendDirect()

    submodules:
        // PDCP-RRC Layer
        pdcpRrc: <LtePdcpRrcType> like LtePdcpRrc {
            @display("p=150,50");
        }
        // RLC Layer
        rlc: LteRlc {
            @display("p=150,150");
        }
        // MAC Layer
        mac: <LteMacType> like LteMac {
            @display("p=150,250");
        }
        // LtePhy submodule
        phy: <LtePhyType> like LtePhy {
            @display("p=150,350");
            @class(LtePhyType);
        }
    connections:
        //#
        //# Connections from LTE Stack to UE/eNB interface
        //#
        EUTRAN_RRC_Sap <--> pdcpRrc.EUTRAN_RRC_Sap;
        upperLayerOut <-- pdcpRrc.DataPort$o;
        upperLayerIn --> pdcpRrc.DataPort$i;


        //#
        //# Internal LTE Stack Connections 
        //# 

        //# PDCP <-> RLC
        pdcpRrc.UM_Sap <--> rlc.UM_Sap;
        pdcpRrc.AM_Sap <--> rlc.AM_Sap;
        pdcpRrc.TM_Sap <--> rlc.TM_Sap;

        //# RLC <-> MAC
        rlc.RLC_to_MAC --> mac.RLC_to_MAC;
        rlc.MAC_to_RLC <-- mac.MAC_to_RLC;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        mac.MAC_to_PHY --> phy.upperGateIn;
        mac.PHY_to_MAC <-- phy.upperGateOut;

        //# external: lower connection
        radioIn --> phy.radioIn;
}

//
// User Equipment of LTE stack
//
module LteNicUe extends LteNicBase
{
    parameters:
        LtePdcpRrcType = "LtePdcpRrcUe";
        LteMacType = "LteMacUe";
        LtePhyType = "LtePhyUe";

    submodules:
        // Feedback generator submodule
        dlFbGen: LteDlFeedbackGenerator {
            @display("p=50,50;is=s");
        }
}

//
// eNodeB of LTE stack
//
module LteNicEnb extends LteNicBase
{
    parameters:
        LtePdcpRrcType = "LtePdcpRrcEnb";
        LteMacType = "LteMacEnb";
        LtePhyType = "LtePhyEnb";

    submodules:
        // TODO Feedback generator submodule
        // ulFbGen: LteUlFeedbackGenerator {
        //     @display("p=200,100;is=s");
        // }
}

//
// Relay of LTE stack
//
module LteNicRelay extends LteNicBase
{
    parameters:
        LtePhyType = "LtePhyRelay";

    submodules:
        // Feedback generator submodule
        dlFbGen: LteDlFeedbackGenerator {
            @display("p=50,50;is=s");
        }

        // TODO Feedback generator submodule
        // ulFbGen: LteUlFeedbackGenerator {
        //     @display("p=200,150;is=s");
        // }
}
