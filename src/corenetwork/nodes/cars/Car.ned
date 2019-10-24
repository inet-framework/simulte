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
        @display("i=device/car;is=vs;bgb=400,518");

        //# Mobility
        mobilityType = default("VeinsInetMobility");

    submodules:
        configurator: HostAutoConfigurator {
            @display("p=49.068,22.968");
        }
}
