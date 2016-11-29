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
import inet.transportlayer.udp.UDP;
import lte.corenetwork.deployer.LteDeployer;
import lte.epc.TrafficFlowFilter;
import lte.epc.gtp.GtpUser;
import lte.stack.phy.LteNicEnb;


//
// eNodeB module with support for EPC (GTP-U, etc).
//
module eNBpp
{
    parameters:
        @networkNode();
        @display("i=device/antennatower;bgb=751,360");

        //# Node specs
        string nodeType = "ENODEB";        // DO NOT CHANGE
        int macNodeId = default(0); // TODO: this is not a real parameter
        int macCellId = default(0);  // TODO: this is not a real parameter
        double txPower @unit(mw) = default(100mw);

        //# Network Layer specs
        string networkLayerType = default("IPv4NetworkLayer");
        string routingTableType = default("IPv4RoutingTable");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");

    gates:
        inout ppp;     // connection to LteIP peer
        input radioIn @directIn;  // connection to slave

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
        // NOTE: instance must be named "lteNic"
        lteNic: LteNicEnb {
            nodeType = nodeType;
            @display("p=200,311");
        }
        pppIf: PPPInterface {
            @display("p=569,311");
        }
        gtp_user: GtpUser {
            @display("p=381,72");
        }
        trafficFlowFilter: TrafficFlowFilter {
            ownerType = nodeType;
            @display("p=200,144");
        }
        udp: UDP {
            @display("p=569,144");
        }
        // network layer
        networkLayer: <networkLayerType> like INetworkLayer {
            parameters:
                @display("p=501,269;q=queue");
        }
    connections allowunconnected:
        //#
        //# Internal LTE stack to PHY and
        //# Lte IP connections
        //#


        //#
        //# External connection to peer Lte Ip and air interface
        //#

        lteNic.radioIn <-- radioIn;
        pppIf.phys <--> ppp;

        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;

        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;

        gtp_user.udpOut --> udp.appIn++;
        gtp_user.udpIn <-- udp.appOut++;

        networkLayer.ifOut++ --> pppIf.upperLayerIn;
        pppIf.upperLayerOut --> networkLayer.ifIn++;

        lteNic.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        gtp_user.pppGate --> lteNic.upperLayerIn;
}
