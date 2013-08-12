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

package lte.simulations.gtpNetwork;

import lte.epc.pgwStandard;
import lte.epc.sgwStandard;
import inet.nodes.inet.Router;
import inet.nodes.inet.StandardHost;
import inet.util.ThruputMeteringChannel;
import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;

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
        iPv4NetworkConfigurator: IPv4NetworkConfigurator {
            @display("p=95,42");
            config = xmldoc("lteCoreConfigFiles/lteIpAddresses.xml");
        }
        pgwStandard: pgwStandard {
            nodeType = "PGW";
            @display("p=746,244");
        }
        eNB3: pgwStandard {
            nodeType = "ENODEB";
            @display("p=223,463");
        }
        eNB1: pgwStandard {
            nodeType = "ENODEB";
            @display("p=223,113");
        }
        eNB2: pgwStandard {
            nodeType = "ENODEB";
            @display("p=223,317");
        }
        sgwStandard2: sgwStandard {
            @display("p=557,370");
        }
        sgwStandard1: sgwStandard {
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
