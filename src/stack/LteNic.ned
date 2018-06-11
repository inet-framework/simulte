// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.stack;

import inet.linklayer.contract.IWirelessNic;
import lte.stack.phy.LtePhy;
import lte.stack.mac.LteMac;
import lte.stack.pdcp_rrc.LtePdcpRrc;
import lte.stack.phy.feedback.LteDlFeedbackGenerator;
import lte.stack.rlc.LteRlc;
import lte.corenetwork.lteip.IP2lte;
import lte.x2.LteX2Manager;
import lte.stack.compManager.LteCompManager;
import lte.stack.d2dModeSelection.D2DModeSelection;
import lte.stack.handoverManager.LteHandoverManager;

import lte.stack.phy.ChannelModel.LteChannelModelInterface;

// 
// Interface for the LTE Stack.
//
moduleinterface ILteNic extends IWirelessNic
{
    string nodeType;
        //bool d2dCapable;            // inherit the value from the parent module
    gates:
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        inout x2[] @loose;     // optional x2 manager
}

// 
// Base module for the LTE Stack.
//
// TODO: implement IWirelessNic
module LteNicBase like ILteNic
{
    parameters:
        @display("i=block/ifcard;bgb=250,444;bgl=3");
        string interfaceTableModule;
        string routingTableModule;
        string LtePdcpRrcType;      // One of: "LtePdcpRrcUe", "LtePdcpRrcEnb", "LtePdcpRrcRelayUe", "LtePdcpRrcRelayEnb"
        string LteMacType;          // One of: "LteMacUe", "LteMacEnb", "LteMacRelayUe", "LteMacRelayEnb"
        string LtePhyType;
        string nodeType;

        string LteChannelModelType = default("LteRealisticChannelModel");

        bool d2dCapable;            // inherit the value from the parent module

    gates:
        //#
        //# Gates connecting UE/eNB and LTE Stack
        //#
        //# Control Ports
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // to receive messages sent using sendDirect()
        inout x2[] @loose;     // optional x2 manager

    submodules:
        // bridge between lte stack and network layer
        ip2lte: IP2lte {
            nodeType = nodeType;
            interfaceTableModule = interfaceTableModule;
            routingTableModule = routingTableModule;
            @display("p=150,68");
        }
        // PDCP-RRC Layer
        pdcpRrc: <LtePdcpRrcType> like LtePdcpRrc {
            @display("p=150,142");
        }
        // RLC Layer
        rlc: LteRlc {
            @display("p=150,227");
            d2dCapable = d2dCapable;
        }
        // MAC Layer
        mac: <LteMacType> like LteMac {
            interfaceTableModule = interfaceTableModule;
            @display("p=151,315");
        }
        // LtePhy submodule
        phy: <LtePhyType> like LtePhy {
            @display("p=151,390");
            @class(LtePhyType);
        }

        channelModel: <LteChannelModelType> like LteChannelModelInterface {
            @display("p=44.8,389.75998");
        }

    connections:
        ip2lte.stackLte <--> pdcpRrc.DataPort;

        //#
        //# Internal LTE Stack Connections 
        //# 

        EUTRAN_RRC_Sap <--> pdcpRrc.EUTRAN_RRC_Sap;

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
        ip2lte.upperLayerOut --> upperLayerOut;
        upperLayerIn --> ip2lte.upperLayerIn;
}

//
// User Equipment of LTE stack
//
module LteNicUe extends LteNicBase
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcUe");
        LteMacType = default("LteMacUe");
        LtePhyType = default("LtePhyUe");
        d2dCapable = default(false);          // DO NOT CHANGE

    submodules:
        // Feedback generator submodule
        dlFbGen: LteDlFeedbackGenerator {
            @display("p=50,50;is=s");
        }
}

//
// D2D-capable User Equipment of LTE stack
//
module LteNicUeD2D extends LteNicUe
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcUeD2D");
        LteMacType = default("LteMacUeD2D");
        LtePhyType = default("LtePhyUeD2D");
        d2dCapable = default(true);             // DO NOT CHANGE
        bool d2dInitialMode = default(false);
}

//
// eNodeB of LTE stack
//
module LteNicEnb extends LteNicBase
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcEnb");
        LteMacType = default("LteMacEnb");
        LtePhyType = default("LtePhyEnb");
        d2dCapable = default(false);          // DO NOT CHANGE
        bool compEnabled = default(false);
        string LteCompManagerType = default("LteCompManagerProportional");

    submodules:
        //#
        //# X2 manager
        //#       
        x2Manager: LteX2Manager {
            @display("p=60,227,row");
        }

        //#
        //# Modules exploiting X2 interface
        //#
        compManager: <LteCompManagerType> like LteCompManager if compEnabled {
            @display("p=60,315,row");
        }
        handoverManager: LteHandoverManager {
            @display("p=60,142,row");
        }

    connections:
        //# connections between X2 Manager and its users
        compManager.x2ManagerIn <-- x2Manager.dataPort$o++ if compEnabled;
        compManager.x2ManagerOut --> x2Manager.dataPort$i++ if compEnabled;
        handoverManager.x2ManagerIn <-- x2Manager.dataPort$o++;
        handoverManager.x2ManagerOut --> x2Manager.dataPort$i++;

        if ( sizeof(x2) > 0 ) {
            //# connections between X2 Manager and GTP (for handover)
            x2$i[0] --> x2Manager.x2Gtp$i;
            x2$o[0] <-- x2Manager.x2Gtp$o;
        }
        //# connections between X2 Manager and the outer world
        for i=1..sizeof(x2)-1 {
            x2$i[i] --> x2Manager.x2$i++;
            x2$o[i] <-- x2Manager.x2$o++;
        }
}

//
// eNodeB of LTE stack with support for D2D-capable UEs
//
module LteNicEnbD2D extends LteNicEnb
{
    parameters:
        LtePdcpRrcType = default("LtePdcpRrcEnbD2D");
        LteMacType = default("LteMacEnbD2D");
        LtePhyType = default("LtePhyEnbD2D");
        d2dCapable = default(true);   // DO NOT CHANGE
        string d2dModeSelectionType = default("D2DModeSelectionBase");

    submodules:
        d2dModeSelection: <d2dModeSelectionType> like D2DModeSelection {
            @display("p=60,68,row");
        }
}

//
// Relay of LTE stack
//
module LteNicRelay extends LteNicBase
{
    parameters:
        LtePhyType = "LtePhyRelay";
        d2dCapable = false;

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
