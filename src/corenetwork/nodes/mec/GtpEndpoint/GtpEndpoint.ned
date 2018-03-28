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


package lte.corenetwork.nodes.mec.GtpEndpoint;

import lte.epc.gtp.GtpUserSimplified;
import inet.linklayer.ppp.PPPInterface;
import inet.node.inet.NodeBase;
import lte.epc.TrafficFlowFilterSimplified;
import inet.transportlayer.udp.UDP;

// 
//# GtpEndpoint module
//
//allowing connection with eNodeB by using the tunnelling
//handled by TrafficFlowFilterSimplified and GtpUserSimplified

module GtpEndpoint extends NodeBase
{
    parameters:
        string nodeType; // must be one between ENB and PGW
        @display("bgb=1496.33,580.415;i=device/mainframe");

    gates:
        inout filterGate @labels(PPPFrame-conn);

    submodules:

        trafficFlowFilter: TrafficFlowFilterSimplified {
            ownerType = nodeType;
            @display("p=1170.015,85.47");
        }
        pppInterface: PPPInterface {
            @display("p=1053.36,405.405");
        }
        udp: UDP {
            @display("p=520.905,85.47");
        }
        gtp_user: GtpUserSimplified {
            @display("p=888.195,85.47");
        }
    connections:
        
		//# connecting UDP Layer with NETWORK Layer
		//
        udp.ipOut --> networkLayer.transportIn++;
        udp.ipIn <-- networkLayer.transportOut++;
        //
        //# connecting the pppInterface to the external-gate to the ME Host (Virtualisation Infrastructure!)
        //
        pppInterface.phys <--> filterGate;


		//# UPLINK --> from ME Host to the eNodeB
		//
		//# from pppInterface to the trafficFlowFilter (UPLINK)
		//
        pppInterface.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        //
        //# from the trafficFlowFilter to the gtpUser (UPLINK)
        //
        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;
        //
        //# from the gtpUser to the UDP Layer (UPLINK)
        //
        gtp_user.udpOut --> udp.appIn++;
        //
        //# at this point the traffic from ME Host to the eNodeB is tunneled at TRANSPORT LAYER!
        
               
        
        //# DOWNLINK --> from eNodeB to the ME Host
		//
		//# from the UDP Layer to the gtpUser (DOWNLINK)
		//
        udp.appOut++ --> gtp_user.udpIn;
        //
        //# from gtpUser to the pppInterface (DOWNLINK)
        //
        pppInterface.upperLayerIn <-- gtp_user.pppGate;
		//
		//# at this point the tunneled traffic is decapsulated and sent to the PPP Layer!
}
