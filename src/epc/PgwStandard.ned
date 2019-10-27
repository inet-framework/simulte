//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//
package lte.epc;

import lte.epc.gtp.GtpUser;
import lte.epc.gtp.GtpUserSimplified;
import inet.common.MessageDispatcher;
import inet.linklayer.ppp.PppInterface;
import inet.node.base.NetworkLayerNodeBase;
import lte.epc.TrafficFlowFilter;
import inet.transportlayer.udp.Udp;

//
// Base Module for a EPC PDN Gateway (P-GW)
//
// Note: Does not include a traffic flow filter or gtp user
//       since those of them are defined in the derived modules.
//
module PgwBase extends NetworkLayerNodeBase
{
    parameters:
        string nodeType; // must be one between ENB and PGW
        @display("bgb=1571.955,755.37;i=device/mainframe");
        forwarding = default(true);
        multicastForwarding = default(false);
        // numPppInterfaces = 1;
        
    gates:
        inout filterGate @labels(PppFrame-conn);

    submodules:
        pppIf: PppInterface {
            @display("p=727,386");
        }
        udp: Udp {
            @display("p=329,206");
        }
        tn: MessageDispatcher {
            parameters:
                @display("p=750,296;b=1000,5,,,,1");
        }
        at: MessageDispatcher {
            parameters:
                @display("p=750,146;b=1000,5,,,,1");
        }
        
        
    connections:
        
        // at dispatcher to transport layer connections
        at.out++ --> udp.appIn;
		at.in++ <-- udp.appOut;
        
        // transport layer to tn dispatcher connections
        udp.ipOut --> tn.in++;
        udp.ipIn <-- tn.out++;

        // tn dispatcher to network layer connections
        tn.out++ --> ipv4.transportIn if hasIpv4;
        tn.in++ <-- ipv4.transportOut if hasIpv4;

        tn.out++ --> ipv6.transportIn if hasIpv6;
        tn.in++ <-- ipv6.transportOut if hasIpv6;

        tn.out++ --> generic.transportIn if hasGn;
        tn.in++ <-- generic.transportOut if hasGn;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;
        
        // pppIf.upperLayerOut --> nl.in++;
        // pppIf.upperLayerIn <-- nl.out++;      
        
        pppIf.phys <--> filterGate;  
}

//
// Implements an EPC PDN Gateway (P-GW)
//
module PgwStandard extends PgwBase 
{
    parameters:
    submodules:
        trafficFlowFilter: TrafficFlowFilter {
            ownerType = nodeType;
            @display("p=813,206");
        }
        gtp_user: GtpUser {
            @display("p=591,206");
        }

	connections:
        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;
        //nl.in++ <-- gtp_user.pppGate;
        
        gtp_user.ipSocketOut --> tn.in++;
        gtp_user.ipSocketIn <-- tn.out++;
        pppIf.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppIf.upperLayerIn <-- gtp_user.pppGate;
        
	    trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;
	    // nl.out++ --> trafficFlowFilter.internetFilterGateIn;
}    

//
// Implements a simplified EPC PDN Gateway (P-GW)
//
module PgwStandardSimplified extends PgwBase 
{
    parameters:
	submodules:
        trafficFlowFilter: TrafficFlowFilterSimplified {
            ownerType = nodeType;
            @display("p=813,206");
        }
        gtp_user: GtpUserSimplified {
            @display("p=591,206");
        }
    connections:
        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;
        //nl.in++ <-- gtp_user.pppGate;
        
	    pppIf.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppIf.upperLayerIn <-- gtp_user.pppGate;
        
	    trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;
	    // nl.out++ --> trafficFlowFilter.internetFilterGateIn;
  }
