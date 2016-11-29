// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes;

import inet.applications.contract.ITCPApp;
import inet.applications.contract.IUDPApp;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.transportlayer.tcp.TCP;
import inet.transportlayer.udp.UDP;
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
        int numTcpApps = default(0);  // no of TCP apps. Specify the app types in INI file with tcpApp[0..1].typename="TCPEchoApp" syntax
        int numUdpApps = default(0);  // no of UDP apps. Specify the app types in INI file with udpApp[0..1].typename="UDPVideoStreamCli" syntax

        //# Node specs
        string nodeType = "UE";  // DO NOT CHANGE
        int masterId;
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0); // TODO: this is not a real parameter
        
        //# D2D capability
        bool d2dCapable = default(false);
        string nicType = d2dCapable ? "LteNicUeD2D" : "LteNicUe";

        //# Network Layer specs
        string networkLayerType = default("IPv4NetworkLayer");
        string routingTableType = default("IPv4RoutingTable");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(absPath(".routingTable"));

    gates:
        input radioIn @directIn;     // connection to master

    submodules:
        interfaceTable: InterfaceTable {
            @display("p=50,75;is=s");
        }
        // routing table
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=50,125;is=s");
        }
        mobility: <mobilityType> like IMobility {
            @display("p=50,175;is=s");
        }
        tcpApp[numTcpApps]: <> like ITCPApp {
            @display("p=175,50,row");
        }
        tcp: TCP if numTcpApps>0 {
            @display("p=175,150");
        }
        udpApp[numUdpApps]: <> like IUDPApp {
            @display("p=325,50,row");
        }
        udp: UDP if numUdpApps>0 {
            @display("p=325,150");
        }
        lteNic: <nicType> like ILteNic {
            nodeType = nodeType;
            d2dCapable = d2dCapable;
            @display("p=250,407");
        }
        // network layer
        networkLayer: <networkLayerType> like INetworkLayer {
            parameters:
                @display("p=250,258");
        }
    connections allowunconnected:
        //# 
        //# Internal TCP/UDP applications connections with IP stack
        //#

        for i=0..numTcpApps-1 {
            tcpApp[i].tcpOut --> tcp.appIn++;
            tcpApp[i].tcpIn <-- tcp.appOut++;
        }

        tcp.ipOut --> networkLayer.transportIn++ if numTcpApps>0;
        tcp.ipIn <-- networkLayer.transportOut++ if numTcpApps>0;

        for i=0..numUdpApps-1 {
            udpApp[i].udpOut --> udp.appIn++;
            udpApp[i].udpIn <-- udp.appOut++;
        }

        udp.ipOut --> networkLayer.transportIn++ if numUdpApps>0;
        udp.ipIn <-- networkLayer.transportOut++ if numUdpApps>0;

        lteNic.radioIn <-- radioIn;

        networkLayer.ifOut++ --> lteNic.upperLayerIn;
        networkLayer.ifIn++ <-- lteNic.upperLayerOut;
}
