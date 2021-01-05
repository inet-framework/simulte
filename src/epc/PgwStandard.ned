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
        @display("bgb=1350,750;i=device/mainframe");
        @figure[applicationLayer](type=rectangle; pos=250,6; size=1000,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");
        @figure[transportLayer](type=rectangle; pos=250,156; size=1000,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=1245,161; anchor=ne; text="transport layer");

        string nodeType; // must be one between ENB and PGW
        forwarding = default(true);
        multicastForwarding = default(false);
        // numPppInterfaces = 1;

    gates:
        inout filterGate @labels(PppFrame-conn);

    submodules:
        pppIf: PppInterface {
            parameters:
                @display("p=1150,525");
        }
        udp: Udp {
            parameters:
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

        pppIf.phys <--> filterGate;
}

//
// Implements an EPC PDN Gateway (P-GW)
//
module PgwStandard extends PgwBase
{
    parameters:
        @figure[submodules];
    submodules:
        trafficFlowFilter: TrafficFlowFilter {
            parameters:
                @display("p=675,80");
                ownerType = nodeType;
        }
        gtp_user: GtpUser {
            parameters:
                @display("p=525,80");
        }

    connections:
        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;

        pppIf.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppIf.upperLayerIn <-- gtp_user.pppGate;

        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;
}

//
// Implements a simplified EPC PDN Gateway (P-GW)
//
module PgwStandardSimplified extends PgwBase
{
    parameters:
        @figure[submodules];
    submodules:
        trafficFlowFilter: TrafficFlowFilterSimplified {
            parameters:
                @display("p=675,80");
                ownerType = nodeType;
        }
        gtp_user: GtpUserSimplified {
            parameters:
                @display("p=525,80");
        }
    connections:
        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;

        pppIf.upperLayerOut --> trafficFlowFilter.internetFilterGateIn;
        pppIf.upperLayerIn <-- gtp_user.pppGate;

        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;
}
