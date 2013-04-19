// 
//                           SimuLTE
// Copyright (C) 2012 Antonio Virdis, Daniele Migliorini, Giovanni
// Accongiagioco, Generoso Pagano, Vincenzo Pii.
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 
package lte.simulations.demo;

import inet.networklayer.autorouting.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.nodes.ethernet.Eth10M;
import inet.nodes.inet.Router;
import inet.nodes.inet.StandardHost;
import inet.world.radio.ChannelControl;
import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.nodes.Ue;
import lte.corenetwork.nodes.eNodeB;


network MultiCell
{
    parameters:
        @display("i=block/network2;bgb=991,558;bgi=background/budapest");
    submodules:
        channelControl: ChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        ipConfigurator: IPv4NetworkConfigurator {
            @display("p=50,125");
            config = xmldoc("multi.xml");
            dumpTopology = true;
        }
        binder: LteBinder {
            @display("p=50,175;is=s");
        }
        server: StandardHost {
            @display("p=212,118;is=n;i=device/server");
        }
        router1: Router {
            @display("p=321,136;i=device/smallrouter");
        }
        router2: Router {
            @display("p=614,116;i=device/smallrouter");
        }
        eNodeB1: eNodeB {
            @display("p=391,259;is=vl");
        }
        eNodeB2: eNodeB {
            @display("p=760,125;is=vl");
        }
        ue11: Ue {
            @display("p=211,271");
        }
        ue12: Ue {
            @display("p=537,424");
        }
        ue21: Ue {
            @display("p=805,484");
        }
        ue22: Ue {
            @display("p=950,271");
        }
    connections:
        server.pppg++ <--> Eth10M <--> router1.pppg++;
        router1.pppg++ <--> Eth10M <--> router2.pppg++;
        router1.pppg++ <--> Eth10M <--> eNodeB1.ppp;
        router2.pppg++ <--> Eth10M <--> eNodeB2.ppp;
}
