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

package lte.simulations.MEC.InetMobility;

import inet.networklayer.configurator.ipv4.IPv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
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

//veins mobility
import org.car2x.veins.subprojects.veins_inet.VeinsInetManager;

//statistics
import lte.apps.d2dMultihop.statistics.MultihopD2DStatistics;

network MecInetThreeCell
{
    parameters:
        @display("bgb=1000, 1000");
        
        int numCars = default(1);
        int numClusters = default(1);
        int numRoads = default(1);

        double playgroundSizeX @unit(m); // x size of the area the nodes are in (in meters)
        double playgroundSizeY @unit(m); // y size of the area the nodes are in (in meters)
        double playgroundSizeZ @unit(m); // z size of the area the nodes are in (in meters)

    submodules:

        routingRecorder: RoutingTableRecorder {	@display("p=985,115;is=s");}
        
        configurator: IPv4NetworkConfigurator {	@display("p=985,195");	config = xmldoc("demo.xml");}
        
        d2dMultihopStatistics: MultihopD2DStatistics { @display("p=985,340;is=s");}
        
        //# LTE modules
        channelControl: LteChannelControl {	@display("p=985,35;is=s");}
        
        binder: LteBinder {	@display("p=985,275;is=s");}

        eNodeB1: eNodeB {	@display("p=100,150;is=vl");}

        eNodeB2: eNodeB {	@display("p=200,450;is=vl");}

        eNodeB3: eNodeB {	@display("p=100,750;is=vl");}

        pgw: PgwStandardSimplified {	@display("p=479.912,610;is=l");	nodeType = "PGW";}

        car[numCars]: Car {	@display("p=300,300");}

        //# MEC module
        meHost: MEHost {	@display("p=481.91998,150;is=l");}

    connections allowunconnected:

        pgw.pppg++ <--> Eth10G <--> eNodeB1.ppp;
        pgw.pppg++ <--> Eth10G <--> eNodeB2.ppp;
        pgw.pppg++ <--> Eth10G <--> eNodeB3.ppp;

        eNodeB1.x2++ <--> Eth10G <--> eNodeB2.x2++;
        eNodeB2.x2++ <--> Eth10G <--> eNodeB3.x2++;

        //#	mec
        // direct connection between eNodeB2 and meHost
        meHost.pppENB++ <--> Eth10G <--> eNodeB1.pppMEHost;
        meHost.pppENB++ <--> Eth10G <--> eNodeB2.pppMEHost;
        meHost.pppENB++ <--> Eth10G <--> eNodeB3.pppMEHost;
}
