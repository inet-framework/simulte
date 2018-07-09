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

package lte.apps.mec.warningAlert;

import lte.corenetwork.nodes.mec.MEPlatform.IMEApp;

// MEWarningAlertApp simple module in charge to:
//
// 1) handling WarningAlertPacket of type INFO_UEAPP by forwarding to the MEWarningAlertService
// 2) handling WarningAlertPacket of type INFO_MEAPP by adding source/destination info and forwarding to the VirtualisationManager

simple MEWarningAlertApp like IMEApp
{
    parameters:
        @display("i=block/app;is=s");
        
        double ram @unit("B") = default(10MB);
        double disk @unit("B") = default(0.1GB);
        double cpu = default(0.001);
               
        string ueSimbolicAddress; 		// i.e. UE[x] or car[x]
        string meHostSimbolicAddress;	// i.e meHost.virtualisationInfrastructure
        
        int packetSize = default(10);
        
        string interfaceTableModule = default("");
           
    gates:
        input virtualisationInfrastructureIn;
        output virtualisationInfrastructureOut;
        
        input mePlatformIn;
        output mePlatformOut;
}

