//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


package lte.corenetwork.nodes;

import inet.linklayer.ppp.PppInterface;
import inet.mobility.static.StationaryMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4RoutingTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.transportlayer.udp.Udp;
import lte.corenetwork.lteCellInfo.LteCellInfo;
import lte.epc.TrafficFlowFilter;
import lte.epc.gtp.GtpUser;
import lte.stack.LteNicEnb;


//
// eNodeB module with support for EPC (GTP-U, etc).
//
module eNBpp extends eNodeBBase
{
    parameters:
        @figure[submodules];
    submodules:
        gtp_user: GtpUser {
            parameters:
            	@display("p=381,72");
            	ipOutInterface = default("cellular");   // FIXME: we should use a different name, eg. "cellular" for the LteNic
        }
        trafficFlowFilter: TrafficFlowFilter {
            parameters:
            	ownerType = nodeType;
            	@display("p=200,144");
        }

    connections allowunconnected:

        nl.in++ <-- gtp_user.pppGate;
        trafficFlowFilter.gtpUserGateOut --> gtp_user.trafficFlowFilterGate;
        trafficFlowFilter.internetFilterGateIn <-- nl.out++;
        gtp_user.socketOut --> at.in++;
        gtp_user.socketIn <-- at.out++;
        
		// set remaining X2 apps connections        
        for i=0..numX2Apps-1 {
            x2App[i].sctpOut[0] --> at.in++;
            x2App[i].sctpIn[0] <-- at.out++;
            x2App[i].sctpOut[1] --> at.in++;
            x2App[i].sctpIn[1] <-- at.out++;
            x2App[i].x2ManagerIn <-- lteNic.x2$o++;
            x2App[i].x2ManagerOut --> lteNic.x2$i++;
        }        
}
