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

package lte.corenetwork.nodes.mec.VirtualisationInfrastructure;

import inet.applications.contract.IUDPApp;

//
//# VirtualisationManager
//	
//	Simple Module for handling all defined UE/ME App Packets!
//
//	The task of this module is:
//		1) to Instantiate a MEAppS according to the StartPacket received & passing the UE destAddress
//		2) mantaining the Data Structure about ALL ME Apps Instantiated!
//		3) to Handle Table mapping UE-IPAddresses & v2vAppName with the CORRESPONDED GATE connected with the MEApp
//		4) forwarding UE/ME App Packets to the right MEApp according to the Table
//

simple VirtualisationManager like IUDPApp{

    parameters:
        @display("i=block/source");
        
        int localPort = default(4000);			// ALL UEApps have to send UDP Packets to this Port!
        int destPort = default(4000);
        
        string interfaceTableModule;
                
    gates:
        
        output udpOut;
        input udpIn;
        output meAppOut[];
        input meAppIn[];
        output resourceManagerOut;
        input resourceManagerIn;
}

