// 
//                           SimuLTE
// Copyright (C) 2015 Antonio Virdis, Giovanni Nardini, Daniele Migliorini, Giovanni
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
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.RoutingTable;
import inet.nodes.inet.NetworkLayer;
import inet.applications.IUDPApp;
import inet.applications.ITCPApp;
import inet.applications.ISCTPApp;
import inet.transport.udp.UDP;
import inet.transport.tcp.TCP;
import inet.transport.sctp.SCTP;
import lte.x2.LteX2App;
import lte.stack.phy.LteNicEnb;
import lte.stack.phy.LteNicEnbX2;
import lte.corenetwork.deployer.LteDeployer;

// 
// eNodeB Module
//
module eNodeB
{
    parameters:
        @node();
        @display("i=device/antennatower;bgb=814,437");

        //# Node specs
        string nodeType = "ENODEB";        // DO NOT CHANGE
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0);  // TODO: this is not a real parameter
        double txPower @unit(mw) = default(100mw);
        bool x2Enabled = default(false);

        //# Num apps
        int numTcpApps = default(0);  // no of TCP apps. Specify the app types in INI file with tcpApp[0..1].typename="TCPEchoApp" syntax
        int numUdpApps = default(0);  // no of UDP apps. Specify the app types in INI file with udpApp[0..1].typename="UDPVideoStreamCli" syntax
        int numX2Apps = default(0);   // no of X2 apps. Specify the app types in INI file with x2App[0..1].typename="X2AppClient" syntax

    gates:
        inout ppp;                // connection to the Core Network
        input radioIn @directIn;  // connection to the radio interface
        inout x2[];               // connection to the X2 interface

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

        //#
        //# NIC modules
        //#
        // NOTE: instance must be named "nic"
        // TODO: rename to something like lte?
        nic: LteNicEnb if !x2Enabled {
            nodeType = nodeType;
            @display("p=330,363");
        }
        nic: LteNicEnbX2 if x2Enabled {
            nodeType = nodeType;
            @display("p=330,363");
        }
        ppp: PPPInterface {
            @display("p=501,363");
        }
        x2ppp[sizeof(x2)]: PPPInterface {
            @display("p=683,363");
        }

        //#
        //# Network layer module
        //#
        networkLayer: NetworkLayer {
            parameters:
                @display("p=501,269;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== UDP ============== //
        udpApp[numUdpApps]: <> like IUDPApp {
            @display("p=501,48,row");
        }
        udp: UDP {
            @display("p=501,148");
        }

        // =============== TCP ============== //
        tcpApp[numTcpApps]: <> like ITCPApp {
            @display("p=683,48,row");
        }
        tcp: TCP if numTcpApps>0 {
            @display("p=683,148");
        }

	    // =============== X2AP ============== //
        x2App[numX2Apps]: LteX2App if x2Enabled {
            @display("p=206,56,row");
        }
        sctp: SCTP if x2Enabled {
            @display("p=330,148");
        }

    connections allowunconnected:

        //#
        //# LTE stack to PHY, network layer and X2 connections
        //#        
        nic.radioIn <-- radioIn;
        nic.upperLayerOut --> networkLayer.ifIn++;
        nic.upperLayerIn <-- networkLayer.ifOut++;

        //#
        //# PPP interface to network layer connections
        //#    
        ppp.phys <--> ppp;
        ppp.upperLayerOut --> networkLayer.ifIn++;
        ppp.upperLayerIn <-- networkLayer.ifOut++;

        //#
        //# X2 interface to network layer connections
        //#    
        for i=0..sizeof(x2)-1 {
            x2ppp[i].phys <--> x2[i];
            x2ppp[i].upperLayerOut --> networkLayer.ifIn++ if x2Enabled;
            x2ppp[i].upperLayerIn <-- networkLayer.ifOut++ if x2Enabled;
        }

		//# 
		//# Apps to transport layer connections
		//#
        for i=0..numTcpApps-1 {
            tcpApp[i].tcpOut --> tcp.appIn++;
            tcpApp[i].tcpIn <-- tcp.appOut++;
        }
        for i=0..numUdpApps-1 {
            udpApp[i].udpOut --> udp.appIn++;
            udpApp[i].udpIn <-- udp.appOut++;
        }
        for i=0..numX2Apps-1, if x2Enabled {
            x2App[i].sctpOut[0] --> sctp.from_appl++;
            x2App[i].sctpIn[0] <-- sctp.to_appl++;

            x2App[i].sctpOut[1] --> sctp.from_appl++;
            x2App[i].sctpIn[1] <-- sctp.to_appl++;
            x2App[i].x2ManagerIn <-- nic.x2$o++;
            x2App[i].x2ManagerOut --> nic.x2$i++;
        }

		//# 
		//# Transport layer to network layer connections
		//#
        tcp.ipOut --> networkLayer.transportIn++ if numTcpApps>0;
        tcp.ipIn <-- networkLayer.transportOut++ if numTcpApps>0;
        udp.ipOut --> networkLayer.transportIn++ if numUdpApps>0;
        udp.ipIn <-- networkLayer.transportOut++ if numUdpApps>0;
        sctp.to_ip --> networkLayer.transportIn++ if (x2Enabled && numX2Apps>0);
        sctp.from_ip <-- networkLayer.transportOut++ if (x2Enabled && numX2Apps>0);
}
