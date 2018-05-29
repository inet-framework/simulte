//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//	@author	Angelo Buono
//


package lte.simulations.mec;


import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.inet.AdhocHost;
import inet.node.inet.Router;
import inet.node.inet.StandardHost;
import inet.node.ethernet.Eth10G;

import lte.world.radio.LteChannelControl;
import lte.epc.PgwStandardSimplified;
import lte.corenetwork.binder.LteBinder;
import lte.corenetwork.nodes.eNodeB;
import lte.corenetwork.nodes.Ue;
import lte.corenetwork.nodes.cars.Car;

//mec
import lte.corenetwork.nodes.mec.MEHost;

import lte.corenetwork.nodes.mec.VirtualisationInfrastructure.VirtualisationInfrastructure;
import lte.corenetwork.nodes.mec.MEPlatform.MEPlatform;

import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;


//
// Derived by cars/Highway.ned Network
//
network Mec
{
    parameters:
        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)
        @display("bgb=882,483");

    submodules:

        routingRecorder: RoutingTableRecorder {
            @display("p=61,75;is=s");
        }
        configurator: IPv4NetworkConfigurator {
            @display("p=61,129");
            config = xmldoc("demo.xml");
        }

        //# Veins manager module
        veinsManager: VeinsInetManager {
            @display("p=61,227;is=s");
        }

        //# LTE modules
        channelControl: LteChannelControl {
            @display("p=62,25;is=s");
        }
        binder: LteBinder {
            @display("p=61,177;is=s");
        }
        server: StandardHost {
            @display("p=857.205,116.2875;is=n;i=device/server");
        }
        router: Router {
            @display("p=714.33746,115.18;i=device/smallrouter");
        }
        pgw: PgwStandardSimplified {
            nodeType = "PGW";
            @display("p=536.02997,116.2875;is=l");
        }
        eNodeB1: eNodeB {
            @display("p=146.19,116.2875;is=vl");
        }
        eNodeB2: eNodeB {
            @display("p=336.68,376.55;is=vl");
        }
        //# MEC module
        meHost: MEHost {
            @display("p=146.19,376.55;is=vl");
        }


    connections allowunconnected:

        server.pppg++ <--> Eth10G <--> router.pppg++;
        router.pppg++ <--> Eth10G <--> pgw.filterGate;

        pgw.pppg++ <--> Eth10G <--> eNodeB1.ppp;
        pgw.pppg++ <--> Eth10G <--> eNodeB2.ppp;

        //# X2 connections
        eNodeB1.x2++ <--> Eth10G <--> eNodeB2.x2++;

        //#	mec
        // direct connection between eNodeB2 and meHost
        meHost.pppENB++ <--> Eth10G <--> eNodeB2.pppMEHost;
        meHost.pppENB++ <--> Eth10G <--> eNodeB1.pppMEHost;

		//meHost.ppp++ <--> Eth10G <--> router.pppg++;
}
