// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 
package lte.simulations.mec;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;

import inet.node.ethernet.Eth10G;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.nodes.eNodeB;
import lte.corenetwork.nodes.Ue;
import lte.epc.PgwStandardSimplified;
import lte.world.radio.LteChannelControl;

network MultiCell_Mec
{
    parameters:
        @display("i=block/network2;bgb=700.252,520.412;bgi=background/budapest");
    submodules:
        channelControl: LteChannelControl {
            @display("p=50,25;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=50,75;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=50,125");
            config = xmldoc("demo.xml");
        }
        binder: LteBinder {
            @display("p=50,175;is=s");
        }
        internet: StandardHost {
            @display("p=198.948,59.572;is=n;i=device/server");
        }
        pgw: PgwStandardSimplified {
            nodeType = "PGW";
            @display("p=386.656,59.572;is=l");
        }
        router1: Router {
            @display("p=277.628,173.096;i=device/smallrouter");
        }
        router2: Router {
            @display("p=488.94,173.096;i=device/smallrouter");
        }

        eNodeB1: eNodeB {
            @display("p=200,300;is=vl");
        }
        eNodeB2: eNodeB {
            @display("p=600,300;is=vl");
        }

        ue1: Ue {
            @display("p=200,350");
        }

        //# MEC
        mec1: StandardHost {
            @display("p=102.284,265.264");
        }
        mec2: StandardHost {
            @display("p=672.152,232.668");
        }

    connections:

        internet.pppg++ <--> Eth10G <--> pgw.filterGate;
        pgw.pppg++ <--> Eth10G <--> router1.pppg++;
        pgw.pppg++ <--> Eth10G <--> router2.pppg++;
        router1.pppg++ <--> Eth10G <--> eNodeB1.ppp;
        router2.pppg++ <--> Eth10G <--> eNodeB2.ppp;
        eNodeB1.pppMec <--> Eth10G <--> mec1.pppg++;
        eNodeB2.pppMec <--> Eth10G <--> mec2.pppg++;

        //# X2 connections
        eNodeB1.x2++ <--> Eth10G <--> eNodeB2.x2++;
}
