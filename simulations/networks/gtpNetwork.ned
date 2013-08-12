//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package lte.simulations.networks;

import lte.epc.pgwStandard;
import inet.examples.bgpv4.BGPOpen.BGPRouterSimple;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.nodes.inet.Router;
import inet.nodes.inet.StandardHost;
import inet.util.ThruputMeteringChannel;


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
        iPv4NetworkConfigurator: IPv4NetworkConfigurator {
            @display("p=95,42");
            config = xmldoc("ipAddresses.xml");
        }
        Host1: StandardHost {
            @display("p=89,121");
        }
        pgwStandard3: pgwStandard {
            nodeType = "PGW";
            @display("p=551,374;i=device/mainframe");
        }
        pgwStandard1: pgwStandard {
            nodeType = "PGW";
            @display("p=89,374");
        }
        router1: Router {
            @display("p=89,262");
        }
        coreNetwork: Router {
            @display("p=313,488");
        }
        pgwStandard2: pgwStandard {
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
