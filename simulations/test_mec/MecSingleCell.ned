//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.simulations.test_mec;


import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth10G;

import lte.world.radio.LteChannelControl;
import lte.epc.PgwStandardSimplified;
import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.nodes.eNodeB;
import lte.corenetwork.nodes.Ue;

//mec
import lte.corenetwork.nodes.mec.MEHost;
import lte.corenetwork.nodes.mec.VirtualisationInfrastructure.VirtualisationInfrastructure;
import lte.corenetwork.nodes.mec.MEPlatform.MEPlatform;


network MecSingleCell
{
    parameters:

        int numUes = default(0);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=1100,500");

    submodules:

        routingRecorder: RoutingTableRecorder {
            @display("p=985,115;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=985,195");
            config = xmldoc("demo.xml");
        }
        ue[numUes]: Ue {
            @display("p=300,300");
        }

        //# LTE modules
        channelControl: LteChannelControl {
            @display("p=985,35;is=s");
        }
        binder: LteBinder {
            @display("p=985,275;is=s");
        }

        eNodeB1: eNodeB {
            @display("p=140,250;is=vl");
        }
        pgw: PgwStandardSimplified {
            nodeType = "PGW";
            @display("p=630,250;is=l");
        }

        //# MEC module
        meHost: MEHost {
            @display("p=400,100;is=l");
        }


    connections allowunconnected:

        pgw.pppg++ <--> Eth10G <--> eNodeB1.ppp;

        //#	mec
        // direct connection between eNodeB2 and meHost
        meHost.pppENB++ <--> Eth10G <--> eNodeB1.pppMEHost;
}
