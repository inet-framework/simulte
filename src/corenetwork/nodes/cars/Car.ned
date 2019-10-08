// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes.cars;

import inet.applications.contract.ITCPApp;
import inet.applications.contract.IApp;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.configurator.ipv4.HostAutoConfigurator;
import inet.transportlayer.tcp.Tcp;
import inet.transportlayer.udp.Udp;
import lte.stack.phy.ILteNic;
import lte.stack.phy.LteNicUe;
import lte.stack.phy.LteNicUeD2D;

// 
// Car Module
//
module Car
{
    parameters:
        @networkNode();
        @display("i=device/car;is=vs;bgb=400,518");

        //# Mobility
        string mobilityType = default("VeinsInetMobility");

        //# Apps
        int numTcpApps = default(0);  // no of Tcp apps
        int numUdpApps = default(0);  // no of Udp apps

        //# Node specs
        string nodeType = "UE";  // DO NOT CHANGE
        int masterId;
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0); // TODO: this is not a real parameter

        //# D2D capability
        string nicType = default("LteNicUe");

        //# Network Layer specs
        string networkLayerType = default("Ipv4NetworkLayer");
        string routingTableType = default("Ipv4RoutingTable");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(absPath(".routingTable"));

    gates:
        input radioIn @directIn;     // connection to master

    submodules:
        mobility: <mobilityType> like IMobility {
            @display("p=50,175;is=s");
        }
        interfaceTable: InterfaceTable {
            @display("p=50,75;is=s");
        }
        // routing table
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=50,125;is=s");
        }
        tcpApp[numTcpApps]: <> like ITCPApp {
            @display("p=175,50,row");
        }
        tcp: Tcp if numTcpApps>0 {
            @display("p=175,150");
        }
        udpApp[numUdpApps]: <> like IApp {
            @display("p=325,50,row");
        }
        udp: Udp if numUdpApps>0 {
            @display("p=325,150");
        }
        // NOTE: instance must be named "lteNic"
        lteNic: <nicType> like ILteNic {
            nodeType = nodeType;
            @display("p=250,407");
        }
        // network layer
        networkLayer: <networkLayerType> like INetworkLayer {
            parameters:
                @display("p=250,258");
        }
        configurator: HostAutoConfigurator {
            @display("p=49.068,22.968");
        }
    connections allowunconnected:
        //# 
        //# Internal Tcp/Udp applications connections with IP stack
        //#

        for i=0..numTcpApps-1 {
            tcpApp[i].tcpOut --> tcp.appIn++;
            tcpApp[i].tcpIn <-- tcp.appOut++;
        }

        tcp.ipOut --> networkLayer.transportIn++ if numTcpApps>0;
        tcp.ipIn <-- networkLayer.transportOut++ if numTcpApps>0;

        for i=0..numUdpApps-1 {
            udpApp[i].socketOut --> udp.appIn++;
            udpApp[i].socketIn <-- udp.appOut++;
        }

        udp.ipOut --> networkLayer.transportIn++ if numUdpApps>0;
        udp.ipIn <-- networkLayer.transportOut++ if numUdpApps>0;

        lteNic.radioIn <-- radioIn;

        networkLayer.ifOut++ --> lteNic.upperLayerIn;
        networkLayer.ifIn++ <-- lteNic.upperLayerOut;
}
