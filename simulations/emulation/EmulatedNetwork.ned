// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 
package lte.simulations.emulation;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.nodes.eNodeB;
import lte.corenetwork.nodes.EmulatedUe;
import lte.world.radio.LteChannelControl;
import lte.epc.PgwStandardSimplified;
import lte.corenetwork.nodes.ExtCell;

// SingleCell_standalone
//
// This network represents the deployment option 3 in 3GPP 38.801
//
network EmulatedNetwork
{
    parameters:
        int numExtCells = default(0);
        @display("i=block/network2;bgb=796,554.7125;bgi=background/pisa");
    submodules:
        routingRecorder: RoutingTableRecorder {
            @display("p=73.38125,130.59375;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=73.38125,72.1375");
            config = xmldoc("demo.xml");
        }
        channelControl: LteChannelControl {
            @display("p=70.89375,189.05;is=s");
        }
        binder: LteBinder {
            @display("p=73.38125,261.1875;is=s");
        }
        router: Router {
            // the router is endowed with an EXT interface
            numExtInterfaces = 1;   
            @display("p=223.875,175.36874;i=device/smallrouter");
        }
        pgw: PgwStandardSimplified {
            nodeType = "PGW";
            @display("p=460.1875,174.125;is=l");
        }
        enb: eNodeB {
            @display("p=671.625,174.125;is=vl");
        }
        ue: EmulatedUe {
            @display("p=671.625,309.69376");
        }
        extCell[numExtCells]: ExtCell {
            @display("p=671.625,432.82498;is=vl");
        }
    connections:
        router.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> enb.ppp;
}
