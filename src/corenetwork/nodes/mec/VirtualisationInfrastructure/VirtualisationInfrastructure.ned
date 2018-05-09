//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//	@author	Angelo Buono
//


package lte.corenetwork.nodes.mec.VirtualisationInfrastructure;

import inet.linklayer.ppp.PPPInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.IPv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.applications.contract.IUDPApp;
import inet.transportlayer.udp.UDP;
import lte.corenetwork.nodes.mec.VirtualisationInfrastructure.VirtualisationManager;


// 
//# VirtualisationInfrastructure
//

module VirtualisationInfrastructure
{
    parameters:
        @networkNode();

        //# Network Layer specs
        string networkLayerType = default("IPv4NetworkLayer");
        string routingTableType = default("IPv4RoutingTable");
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default(routingTableType != "" ? absPath(".routingTable") : "");

    gates:

        inout ppp[];

        output meAppOut[];			// connection to the ME App input gate
        input meAppIn[];			// connection to the ME App output gate

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=61.344,74.976;is=s");
        }

        // TODO: should not include a routing table
        // routing table
        routingTable: <routingTableType> like IRoutingTable if routingTableType != "" {
            parameters:
                @display("p=61.344,124.96;is=s");
        }

        //#
        //# ppp modules
        //#

        pppIf[sizeof(ppp)]: PPPInterface {
            @display("p=252,473");
        }

        //#
        //# Network layer module
        //#
        networkLayer: <networkLayerType> like INetworkLayer {
            parameters:
                @display("p=252,343;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== UDP ============== //
        virtualisationManager: VirtualisationManager {
            @display("p=252,124,row");
        }

        udp: UDP {
            @display("p=252,231");
        }

        //#
        //# Resource Manager module
        //#
        resourceManager: ResourceManager {
            @display("p=430,124,row");
        }

    connections allowunconnected:

        //#
        //# PPP interface to network layer connections
        //# 
        for i=0..sizeof(ppp)-1 {
            pppIf[i].phys <--> ppp++;
            pppIf[i].upperLayerOut --> networkLayer.ifIn++;
            pppIf[i].upperLayerIn <-- networkLayer.ifOut++;
        }

        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;

        //# 
        //# VirtualisationManager to transport layer connections
        //#
        virtualisationManager.udpIn <-- udp.appOut++;
        virtualisationManager.udpOut --> udp.appIn++;

        //# 
        //# VirtualisationManager to ResourceManager connections
        //#
        virtualisationManager.resourceManagerIn <-- resourceManager.virtualisationManagerOut;
        virtualisationManager.resourceManagerOut --> resourceManager.virtualisationManagerIn;


        //# 
        //# Internal connections: meAppIn & meAppOut gates to virtualisationManager.meAppIn & virtualisationManager.meAppOut
        //#
        //	IN VirtualisationManager initialize()
}
