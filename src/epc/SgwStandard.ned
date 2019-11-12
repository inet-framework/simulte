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
import inet.common.MessageDispatcher;
import inet.linklayer.ppp.PppInterface;
import inet.node.base.NetworkLayerNodeBase;
import inet.transportlayer.udp.Udp;

//
// Implements an EPC Serving Gateway (S-GW)
//
module SgwStandard extends NetworkLayerNodeBase
{
    parameters:
        @display("bgb=1250,700;i=device/server2");
        @figure[applicationLayer](type=rectangle; pos=250,6; size=1000,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=1245,11; anchor=ne; text="application layer");
        @figure[transportLayer](type=rectangle; pos=250,156; size=1000,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=1245,161; anchor=ne; text="transport layer");
        @figure[submodules];
        string nodeType = "SGW";  // DO NOT CHANGE
        forwarding = default(true);
        multicastForwarding = default(true);

    submodules:
        udp: Udp {
            @display("p=329,206");
        }
        gtp_user: GtpUser {
            parameters:
                @display("p=727,206");
                filter = false; // the sgw has no traffic flow filter
                tftFileName = "";
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
        udp.ipOut --> tn.in++;
        udp.ipIn <-- tn.out++;

        tn.out++ --> ipv4.transportIn; //if hasIpv4;  //FIXME: set parameter
        tn.in++ <-- ipv4.transportOut; //if hasIpv4;

        tn.out++ --> ipv6.transportIn if hasIpv6;
        tn.in++ <-- ipv6.transportOut if hasIpv6;

        tn.out++ --> generic.transportIn if hasGn;
        tn.in++ <-- generic.transportOut if hasGn;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;

        at.out++ --> udp.appIn;
        at.in++ <-- udp.appOut;

        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;

}
