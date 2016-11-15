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
import lte.world.radio.LteChannelControl;


//
// TODO auto-generated type
//
network GtpNetwork
{
    @display("bgb=658,529");
    types:
        channel networkConnection extends ThruputMeteringChannel
        {
            delay = 0.1us;
            datarate = 10Mbps;
            thruputDisplayFormat = "u";
        }
    submodules:
        Host3: StandardHost {
            @display("p=504,121");
        }
        router3: Router {
            @display("p=551,262");
        }
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=95,42");
            config = xmldoc("ipAddresses.xml");
        }
        Host1: StandardHost {
            @display("p=89,121");
        }
        pgwStandard3: PgwStandard {
            nodeType = "PGW";
            @display("p=551,374;i=device/mainframe");
        }
        pgwStandard1: PgwStandard {
            nodeType = "PGW";
            @display("p=89,374");
        }
        router1: Router {
            @display("p=89,262");
        }
        coreNetwork: Router {
            @display("p=313,488");
        }
        pgwStandard2: PgwStandard {
            nodeType = "PGW";
            @display("p=313,374");
        }
        router2: Router {
            @display("p=313,262");
        }
        Host2: StandardHost {
            @display("p=313,121");
        }
        Host4: StandardHost {
            @display("p=598,121");
        }
    connections:
        router3.ethg++ <--> networkConnection <--> Host3.ethg++;
        router3.pppg++ <--> networkConnection <--> pgwStandard3.filterGate;
        pgwStandard1.filterGate <--> networkConnection <--> router1.pppg++;
        router1.ethg++ <--> networkConnection <--> Host1.ethg++;
        Host2.ethg++ <--> networkConnection <--> router2.ethg++;
        router2.pppg++ <--> networkConnection <--> pgwStandard2.filterGate;
        pgwStandard1.pppg++ <--> networkConnection <--> coreNetwork.pppg++;
        coreNetwork.pppg++ <--> networkConnection <--> pgwStandard3.pppg++;
        pgwStandard2.pppg++ <--> networkConnection <--> coreNetwork.pppg++;
        router3.ethg++ <--> networkConnection <--> Host4.ethg++;
}
