// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes;

import inet.applications.contract.IApp;
import inet.applications.contract.IApp;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.transportlayer.contract.ITcp;
import inet.transportlayer.contract.IUdp;
import inet.transportlayer.contract.ISctp;
import inet.common.MessageDispatcher;
import lte.stack.phy.ILteNic;
import lte.stack.phy.LteNicUe;
import lte.stack.phy.LteNicUeD2D;

// 
// User Equipment Module
//
module Ue
{
    parameters:
        @networkNode();
        @display("i=device/pocketpc;bgb=400,518");

        //# Mobility
        string mobilityType = default("StationaryMobility");

        //# Apps
        // FIXME: generic IApp interface - simulation inifiles need to be adapted!
        // int numTcpApps = default(0);  // no of Tcp apps. Specify the app types in INI file with tcpApp[0..1].typename="TcpEchoApp" syntax
        // int numUdpApps = default(0);  // no of Udp apps. Specify the app types in INI file with udpApp[0..1].typename="UdpVideoStreamClient" syntax
        int numApps = default(0);  // no of apps. 
        
        //# Transport layer
        bool hasUdp = default(firstAvailableOrEmpty("Udp") != "");
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = default(false);
        
        //# Node specs
        string nodeType = "UE";  // DO NOT CHANGE
        int masterId;
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0); // TODO: this is not a real parameter
        
        //# D2D capability
        string nicType = default("LteNicUe");

        //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default("^.ipv4.routingTable");

    gates:
        input radioIn @directIn;     // connection to master

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=50,75;is=s");
        }
        mobility: <mobilityType> like IMobility {
            @display("p=50,175;is=s");
        }
        app[numApps]: <> like IApp {
            @display("p=175,50,row");
        }
        udp: <default(firstAvailableOrEmpty("Udp"))> like IUdp if hasUdp {
            parameters:
                @display("p=375,226");
        }
        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITcp if hasTcp {
            parameters:
                @display("p=525,226");
        }
        sctp: <default(firstAvailableOrEmpty("Sctp"))> like ISctp if hasSctp {
            parameters:
                @display("p=675,226");
        }
        // network layer
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=375,376;q=queue");
        }
        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
            parameters:
                @display("p=525,376;q=queue");
        }
        lteNic: <nicType> like ILteNic {
            nodeType = nodeType;
            //d2dCapable = d2dCapable;
            @display("p=250,407");
        }
        // message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=750,146;b=1000,5,,,,1");
        }
        // message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=750,296;b=1000,5,,,,1");
        }
        // message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=750,446;b=1000,5,,,,1");
        }
    connections allowunconnected:
        //# LTE Nic 
        lteNic.radioIn <-- radioIn;

        lteNic.upperLayerOut --> nl.in++;
        lteNic.upperLayerIn <-- nl.out++;
        
        //# 
        //# Internal Tcp/Udp applications connections with transport layer
        //#

        for i=0..numApps-1 {
            app[i].socketOut --> at.in++;
            app[i].socketIn <-- at.out++;
        }

		at.out++ --> udp.appIn if hasUdp;
		at.in++ <-- udp.appOut if hasUdp;
		
		at.out++ --> tcp.appIn if hasTcp;
		at.in++ <-- tcp.appOut if hasTcp;
		
		at.out++ --> sctp.appIn if hasSctp;
		at.in++ <-- sctp.appOut if hasSctp;

        //# 
		//# Internal transport layer connections to network layer
		//# 
	
        udp.ipOut --> tn.in++ if hasUdp;
        udp.ipIn <-- tn.out++ if hasUdp;

        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;

        sctp.ipOut --> tn.in++ if hasSctp;
        tn.out++ --> sctp.ipIn if hasSctp;
           
        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;
        
        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;    
        
        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;
      
        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;
        
        at.out++ --> tn.in++;
        at.in++ <-- tn.out++;
}
