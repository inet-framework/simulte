//
//                           SimuLTE
// Copyright (C) 2013 Antonio Virdis
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.simulations.networks;

import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.nodes.inet.Router;
import inet.nodes.inet.StandardHost;
import inet.util.ThruputMeteringChannel;
import inet.world.radio.ChannelControl;
import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.nodes.Ue;
import lte.corenetwork.nodes.eNBpp;
import lte.epc.PgwStandard;
import lte.epc.SgwStandard;


network eutran_epcNetwork
{
    parameters:
        int numUe = default(1);
        @display("i=block/network2;bgb=798,558;bgi=background/terrain");
    types:
        channel networkConnection extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 10Mbps;
            thruputDisplayFormat = "u";
        }
    submodules:
        channelControl: ChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=434,25;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=183,25");
            config = xmldoc("lteCoreConfigFiles/lteIpAddresses.xml");
        }
        binder: LteBinder {
            @display("p=310,25;is=s");
        }
        eNodeB: eNBpp {
            @display("p=54,291;is=vl");
        }
        ue[numUe]: Ue {
            @display("p=69,177");
        }
        router1: Router {
            @display("p=183,501");
        }
        InternetHost: StandardHost {
            @display("p=735,144");
        }
        pgwStandard: PgwStandard {
            nodeType = "PGW";
            @display("p=542,291");
        }
        router: Router {
            @display("p=442,501");
        }
        router3: Router {
            @display("p=735,291");
        }
        sgwStandard1: SgwStandard {
            @display("p=300,291");
        }
    connections:
        router1.pppg++ <--> networkConnection <--> eNodeB.ppp;
        InternetHost.ethg++ <--> networkConnection <--> router3.ethg++;

        pgwStandard.pppg++ <--> networkConnection <--> router.pppg++;
        pgwStandard.filterGate <--> networkConnection <--> router3.pppg++;

        router.pppg++ <--> networkConnection <--> sgwStandard1.pppg++;
        router1.pppg++ <--> networkConnection <--> sgwStandard1.pppg++;
}
