
package lte.corenetwork.nodes;

import inet.base.NotificationBoard;
import inet.linklayer.ppp.PPPInterface;
import inet.mobility.models.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.RoutingTable;
import inet.nodes.inet.NetworkLayer;
import lte.corenetwork.deployer.LteDeployer;
import lte.corenetwork.lteip.LteIp;
import lte.stack.phy.LteNicEnb;
import lte.epc.gtp.GtpUser;
import inet.transport.udp.UDP;
import lte.epc.TrafficFlowFilter;

// 
// eNodeB Module
//
module eNBpp
{
    parameters:
        @node();
        @display("i=device/antennatower;bgb=751,360");

        //# Node specs
        string nodeType = "ENODEB";		// DO NOT CHANGE
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
        // TODO: should be replaced with a relay?
        lteIp: LteIp {
            @display("p=200,225");
            nodeType = nodeType;
        }
        // NOTE: instance must be named "nic"
        // TODO: rename to something like lte?
        nic: LteNicEnb {
            @display("p=200,311");
        }
        ppp: PPPInterface {
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
        networkLayer: NetworkLayer {
            @display("p=569,225");
        }
    connections allowunconnected:
        //#
		//# Internal LTE stack to PHY and
		//# Lte IP connections
		//#

        lteIp.stackLte$i <-- nic.upperLayerOut;			// LTE stack data port
        lteIp.stackLte$o --> nic.upperLayerIn;         // LTE stack data port

		//# 
        //# External connection to peer Lte Ip and air interface
        //#

        nic.radioIn <-- radioIn;

        ppp.phys <--> ppp;

        udp.ipOut --> networkLayer.udpIn;
        udp.ipIn <-- networkLayer.udpOut;

        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;

        lteIp.ifOut --> trafficFlowFilter.internetFilterGateIn;

        gtp_user.pppGate --> lteIp.ifIn;
        gtp_user.udpOut --> udp.appIn++;
        gtp_user.udpIn <-- udp.appOut++;

        networkLayer.ifOut++ --> ppp.upperLayerIn;
        ppp.upperLayerOut --> networkLayer.ifIn++;
}
