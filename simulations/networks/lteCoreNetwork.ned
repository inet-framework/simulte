//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

package lte.simulations.networks;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.common.misc.ThruputMeteringChannel;
import lte.epc.PgwStandard;
import lte.epc.SgwStandard;
import lte.world.radio.LteChannelControl;


network lteCoreNetwork
{
    @display("bgb=1154,598");
    types:
        channel networkConnection extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 10Mbps;
            thruputDisplayFormat = "u";
        }
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=95,42");
            config = xmldoc("lteCoreConfigFiles/lteIpAddresses.xml");
        }
        pgwStandard: PgwStandard {
            nodeType = "PGW";
            @display("p=746,244");
        }
        eNB3: PgwStandard {
            nodeType = "ENODEB";
            @display("p=223,463");
        }
        eNB1: PgwStandard {
            nodeType = "ENODEB";
            @display("p=223,113");
        }
        eNB2: PgwStandard {
            nodeType = "ENODEB";
            @display("p=223,317");
        }
        sgwStandard2: SgwStandard {
            @display("p=557,370");
        }
        sgwStandard1: SgwStandard {
            @display("p=557,113");
        }
        router: Router {
            @display("p=557,244");
        }
        router1: Router {
            @display("p=381,113");
        }
        router2: Router {
            @display("p=381,370");
        }
        router3: Router {
            @display("p=928,244");
        }
        InternetHost: StandardHost {
            @display("p=1096,244");
        }
        UE1: StandardHost {
            @display("p=37,113;i=device/cellphone");
        }
        UE2: StandardHost {
            @display("p=37,317;i=device/cellphone");
        }
        UE3: StandardHost {
            @display("p=37,463;i=device/cellphone");
        }
    connections:
        pgwStandard.pppg++ <--> networkConnection <--> router.pppg++;
        pgwStandard.filterGate <--> networkConnection <--> router3.pppg++;

        router.pppg++ <--> networkConnection <--> sgwStandard2.pppg++;
        router2.pppg++ <--> networkConnection <--> sgwStandard2.pppg++;

        router.pppg++ <--> networkConnection <--> sgwStandard1.pppg++;
        router1.pppg++ <--> networkConnection <--> sgwStandard1.pppg++;

        router2.pppg++ <--> networkConnection <--> eNB3.pppg++;
        router2.pppg++ <--> networkConnection <--> eNB2.pppg++;
        router1.pppg++ <--> networkConnection <--> eNB1.pppg++;

        InternetHost.ethg++ <--> networkConnection <--> router3.ethg++;

        UE3.pppg++ <--> networkConnection <--> eNB3.filterGate;
        UE2.pppg++ <--> networkConnection <--> eNB2.filterGate;
        UE1.pppg++ <--> networkConnection <--> eNB1.filterGate;
}
