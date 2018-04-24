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


package lte.corenetwork.nodes.mec.MEPlatform;

import lte.corenetwork.nodes.mec.MEPlatform.IMEService;

// 
//# MEPlatform
//

module MEPlatform
{
    parameters:

		//# Num Services
        int numServices = default(0); // no of UDP ME Services. Specify the service types in INI file with udpManager.udpService[0..1].typename="UDPServ" syntax

    gates:
        
        input meAppIn[];
        output meAppOut[];

    submodules:
        
        udpService[numServices]: <> like  IMEService{
            @display("p=555,48,row");
        }  
    
    connections allowunconnected:
}

