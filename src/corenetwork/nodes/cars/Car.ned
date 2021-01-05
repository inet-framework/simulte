// 
//                           SimuLTE
// 
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package lte.corenetwork.nodes.cars;

import inet.applications.contract.IApp;
import inet.mobility.contract.IMobility;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.contract.IRoutingTable;
import inet.networklayer.contract.INetworkLayer;
import inet.networklayer.configurator.ipv4.HostAutoConfigurator;
import inet.transportlayer.tcp.Tcp;
import inet.transportlayer.udp.Udp;
import inet.common.MessageDispatcher;
import lte.corenetwork.nodes.Ue;
import lte.stack.ILteNic;

// 
// Car Module
//
module Car extends Ue
{
    parameters:
        @display("i=device/car");

        //# Mobility
        mobilityType = default("VeinsInetMobility");

        //# Network Layer specs
        *.routingTableModule = default(absPath(".ipv4.routingTable"));
        
        ipv4.configurator.typename = "HostAutoConfigurator";
        ipv4.configurator.interfaces = "cellular";
        
    submodules:
	    configurator: HostAutoConfigurator {
            @display("p=126.4,413.96");
        }
}
