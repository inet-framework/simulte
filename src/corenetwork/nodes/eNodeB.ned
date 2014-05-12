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


package lte.corenetwork.nodes;

import inet.base.NotificationBoard;
import inet.linklayer.ppp.PPPInterface;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.autorouting.ipv4.IPv4NodeConfigurator;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.RoutingTable;
import lte.corenetwork.deployer.LteDeployer;
import lte.stack.phy.LteNicEnb;


// 
// eNodeB Module
//
module eNodeB
{
    parameters:
        @node();
        @display("i=device/antennatower;bgb=300,300");

        //# Node specs
        string nodeType = "ENODEB";        // DO NOT CHANGE
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0);  // TODO: this is not a real parameter
        double txPower @unit(mw) = default(100mw);

    gates:
        inout ppp;     // connection to LteIP peer
        input radioIn @directIn;  // connection to slave

    submodules:
        notificationBoard: NotificationBoard {
            @display("p=50,25;is=s");
        }
        interfaceTable: InterfaceTable {
            @display("p=50,75;is=s");
        }
        // TODO: should not include a routing table
        routingTable: RoutingTable {
            @display("p=50,125;is=s");
        }
        mobility: StationaryMobility {
            @display("p=50,175;is=s");
        }
        // NOTE: LteDeployer must be created first
        deployer: LteDeployer {
            @display("p=50,225;is=s");
        }
        // NOTE: instance must be named "nic"
        // TODO: rename to something like lte?
        nic: LteNicEnb {
            nodeType = nodeType;
            @display("p=150,250");
        }
        ppp: PPPInterface {
            @display("p=250,250");
        }
        nodeConfigurator: IPv4NodeConfigurator {
            @display("p=50,272");
        }
    connections allowunconnected:
        //#
        //# Internal LTE stack to PHY and
        //# Lte IP connections
        //#

        nic.radioIn <-- radioIn;
        ppp.phys <--> ppp;
        nic.upperLayerOut --> ppp.upperLayerIn;
        ppp.upperLayerOut --> nic.upperLayerIn;
}
