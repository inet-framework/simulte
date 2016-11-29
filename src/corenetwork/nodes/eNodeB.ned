// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes;

import inet.linklayer.ppp.PPPInterface;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.IPv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IUDPApp;
import inet.applications.contract.ITCPApp;
import inet.applications.contract.ISCTPApp;
import inet.transportlayer.udp.UDP;
import inet.transportlayer.tcp.TCP;
import inet.transportlayer.sctp.SCTP;
import lte.x2.LteX2App;
import lte.stack.phy.ILteNic;
import lte.stack.phy.LteNicEnb;
import lte.stack.phy.LteNicEnbD2D;
import lte.corenetwork.deployer.LteDeployer;
import lte.epc.gtp.GtpUserSimplified;
import lte.epc.TrafficFlowFilterSimplified;
import lte.epc.gtp.GtpUserX2;

// 
// eNodeB Module
//
module eNodeB
{
    parameters:
        @networkNode();
        @display("i=device/antennatower;bgb=814,437");

        //# Node specs
        string nodeType = "ENODEB";        // DO NOT CHANGE
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0);  // TODO: this is not a real parameter
        double txPower @unit(mw) = default(100mw);
        bool d2dCapable = default(false);
        string nicType = d2dCapable ? "LteNicEnbD2D" : "LteNicEnb";

        //# Network Layer specs
        string networkLayerType = default("IPv4NetworkLayer");
        string routingTableType = default("IPv4RoutingTable");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");

        //# Num apps
        int numTcpApps = default(0);  // no of TCP apps. Specify the app types in INI file with tcpApp[0..1].typename="TCPEchoApp" syntax
        int numUdpApps = default(0);  // no of UDP apps. Specify the app types in INI file with udpApp[0..1].typename="UDPVideoStreamCli" syntax
        int numX2Apps = default(0);   // no of X2 apps. Specify the app types in INI file with x2App[0..1].typename="X2AppClient" syntax

    gates:
        inout ppp;                // connection to the Core Network
        input radioIn @directIn;  // connection to the radio interface
        inout x2[];               // connection to the X2 interface

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=50,75;is=s");
        }
        // TODO: should not include a routing table
        // routing table
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
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
        //# lteNic modules
        //#
        lteNic: <nicType> like ILteNic {
            nodeType = nodeType;
            d2dCapable = d2dCapable;
            @display("p=250,407");
        }

        pppIf: PPPInterface {
            @display("p=501,363");
        }
        
        x2ppp[sizeof(x2)]: PPPInterface {
            @display("p=683,363");
        }

        //#
        //# Network layer module
        //#
        networkLayer: <networkLayerType> like INetworkLayer {
            parameters:
                @display("p=501,269;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== UDP ============== //
        udpApp[numUdpApps]: <> like IUDPApp {
            @display("p=555,48,row");
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
        x2App[numX2Apps]: LteX2App {
            @display("p=155,48,row");
        }
        sctp: SCTP {
            @display("p=275,148");
        }

        //# 
        //# Simplified EPC
        //# 
        trafficFlowFilter: TrafficFlowFilterSimplified {
            ownerType = nodeType;
            @display("p=330,48");
        }
        gtpUser: GtpUserSimplified {
            @display("p=434,48");
        }

        gtpUserX2: GtpUserX2 {
            @display("p=410,363");
        }

    connections allowunconnected:

        //#
        //# LTE stack to PHY and network layer
        //#        
        lteNic.radioIn <-- radioIn;
        lteNic.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        lteNic.upperLayerIn <-- gtpUser.pppGate;
        trafficFlowFilter.gtpUserGateOut --> gtpUser.trafficFlowFilterGate;
        gtpUser.udpOut --> udp.appIn++;
        gtpUser.udpIn <-- udp.appOut++;


        //#
        //# PPP interface to network layer connections
        //#    
        pppIf.phys <--> ppp;
        pppIf.upperLayerOut --> networkLayer.ifIn++;
        pppIf.upperLayerIn <-- networkLayer.ifOut++;


        gtpUserX2.lteStackOut --> lteNic.x2$i++;
        gtpUserX2.lteStackIn <-- lteNic.x2$o++;
        gtpUserX2.udpOut --> udp.appIn++;
        gtpUserX2.udpIn <-- udp.appOut++;

        //#
        //# X2 interface to network layer connections
        //#    
        for i=0..sizeof(x2)-1 {
            x2ppp[i].phys <--> x2[i];
            x2ppp[i].upperLayerOut --> networkLayer.ifIn++;
            x2ppp[i].upperLayerIn <-- networkLayer.ifOut++;
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
        for i=0..numX2Apps-1 {
            x2App[i].sctpOut[0] --> sctp.from_appl++;
            x2App[i].sctpIn[0] <-- sctp.to_appl++;

            x2App[i].sctpOut[1] --> sctp.from_appl++;
            x2App[i].sctpIn[1] <-- sctp.to_appl++;
            x2App[i].x2ManagerIn <-- lteNic.x2$o++;
            x2App[i].x2ManagerOut --> lteNic.x2$i++;
        }

        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;
        tcp.ipOut --> networkLayer.transportIn++ if numTcpApps>0;
        tcp.ipIn <-- networkLayer.transportOut++ if numTcpApps>0;
        sctp.to_ip --> networkLayer.transportIn++ if numX2Apps>0;
        sctp.from_ip <-- networkLayer.transportOut++ if numX2Apps>0;
}
