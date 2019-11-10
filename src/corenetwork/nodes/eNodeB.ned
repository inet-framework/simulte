// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes;

import inet.linklayer.ppp.PppInterface;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IApp;
import inet.transportlayer.udp.Udp;
import inet.transportlayer.tcp.Tcp;
import inet.transportlayer.sctp.Sctp;
import inet.common.MessageDispatcher;
import lte.x2.LteX2App;
import lte.stack.ILteNic;
import lte.corenetwork.lteCellInfo.LteCellInfo;
import lte.epc.gtp.GtpUserSimplified;
import lte.epc.TrafficFlowFilterSimplified;
import lte.epc.gtp.GtpUserX2;

// 
// eNodeB Base Module
// 
// Base module for eNodeB functionality - except for gtpuser and trafficflow,
// which are defined in the derived submodules.
//
module eNodeBBase
{
    parameters:
        @networkNode();
        @display("i=device/antennatower;bgb=814,437");

        //# Node specs
        string nodeType = "ENODEB";  // DO NOT CHANGE
        int macNodeId = default(0);  // TODO: this is not a real parameter
        int macCellId = default(0);  // TODO: this is not a real parameter
        double txPower @unit(mw) = default(100mw);
        string nicType = default("LteNicEnb");

        //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default("^.ipv4.routingTable");
        *.forwarding = true;
        *.multicastForwarding = false;

        //# Apps
        int numApps = default(0);     // no of apps. 
        int numX2Apps = default(0);   // no of X2 apps. Specify the app types in INI file with x2App[0..1].typename="X2AppClient" syntax
        
        //# Transport layer
        bool hasUdp = true;
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = true;

    gates:
        inout ppp;                // connection to the Core Network
        input radioIn @directIn;  // connection to the radio interface
        inout x2[];               // connection to the X2 interface

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=50,75;is=s");
        }
        mobility: StationaryMobility {
            @display("p=50,175;is=s");
        }
        cellInfo: LteCellInfo {
            @display("p=50,225;is=s");
        }

        //#
        //# lteNic modules
        //#
        lteNic: <nicType> like ILteNic {
            nodeType = nodeType;
            //d2dCapable = d2dCapable;
            @display("p=250,407");
        }
        
        pppIf: PppInterface {
            @display("p=501,363");
        }
        
        x2ppp[sizeof(x2)]: PppInterface {
            @display("p=683,363");
        }
        
        //#
        //# Network layer module
        //#
        ipv4: <default("Ipv4NetworkLayer")> like INetworkLayer if hasIpv4 {
            parameters:
                @display("p=375,376;q=queue");
        }
        ipv6: <default("Ipv6NetworkLayer")> like INetworkLayer if hasIpv6 {
            parameters:
                @display("p=525,376;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== Udp ============== //
        app[numApps]: <> like IApp {
            @display("p=555,48,row");
        }
        udp: Udp {
            @display("p=501,148");
        }
        tcp: Tcp if hasTcp {
            @display("p=683,148");
        }

	    // =============== X2AP ============== //
        x2App[numX2Apps]: LteX2App {
            @display("p=155,48,row");
        }
        sctp: Sctp {
            @display("p=275,148");
        }

	    // message dispatcher for SAP to transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=750,146;b=1000,5,,,,1");
        }
        
        // message dispatcher for SAP to network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=750,146;b=1000,5,,,,1");
        }
       
        // message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=750,446;b=1000,5,,,,1");
        }
       
        

    connections allowunconnected:

        //#
        //# LTE stack to PHY and network layer
        //#        
        lteNic.radioIn <-- radioIn;

        //#
        //# Ppp interface to network layer connections
        //#    
        pppIf.phys <--> ppp;
        pppIf.upperLayerOut --> nl.in++;
        pppIf.upperLayerIn <-- nl.out++;        

        //#
        //# X2 interface to network layer connections
        //#    
        for i=0..sizeof(x2)-1 {
            x2ppp[i].phys <--> x2[i];
            x2ppp[i].upperLayerOut --> tn.in++;  
            x2ppp[i].upperLayerIn <-- tn.out++;
        }

		//# 
		//# Apps to transport layer connections
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
        //# Transport layer to network layer connections
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
        
        lteNic.upperLayerIn <-- nl.out++;
        lteNic.upperLayerOut --> nl.in++;
}


// eNodeB with simplified gtpuser and trafficflow
//
module eNodeB extends eNodeBBase
{
    submodules:
        //# 
        //# Simplified EPC
        //# 
        trafficFlowFilter: TrafficFlowFilterSimplified {
            ownerType = nodeType;
            @display("p=330,48");
        }
        gtpUser: GtpUserSimplified {
            @display("p=434,48");
            ipOutInterface = "wlan";       // FIXME: we should use a different name, eg. "cellular" for the LteNic
        }

        gtpUserX2: GtpUserX2 {
            @display("p=410,363");
        }
    connections:
        nl.in++ <-- gtpUser.pppGate;
        trafficFlowFilter.gtpUserGateOut --> gtpUser.trafficFlowFilterGate;
        trafficFlowFilter.internetFilterGateIn <-- nl.out++;
        gtpUser.socketOut --> at.in++;
        gtpUser.socketIn <-- at.out++;
        
	    // connections for the X2 interface
        gtpUserX2.lteStackOut --> lteNic.x2$i++;
        gtpUserX2.lteStackIn <-- lteNic.x2$o++;
        gtpUserX2.socketOut --> at.in++;
        gtpUserX2.socketIn <-- at.out++;
        
        // set remaining X2 apps connections        
        for i=0..numX2Apps-1 {
            x2App[i].sctpOut[0] --> at.in++;
            x2App[i].sctpIn[0] <-- at.out++;
            x2App[i].sctpOut[1] --> at.in++;
            x2App[i].sctpIn[1] <-- at.out++;
            x2App[i].x2ManagerIn <-- lteNic.x2$o++;
            x2App[i].x2ManagerOut --> lteNic.x2$i++;
        }
    
}
