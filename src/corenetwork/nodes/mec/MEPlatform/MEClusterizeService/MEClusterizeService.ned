//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

//
//  @author Angelo Buono
//

package lte.corenetwork.nodes.mec.MEPlatform.MEClusterizeService;

import lte.corenetwork.nodes.mec.MEPlatform.IMEService;

//
//
//

simple MEClusterizeService like IMEService
{
    parameters:
        
        //# mode how to send the cluster configuration to the UEClusterizeApp
        // V2V_UNICAST_TX_MODE :				0
		// V2V_MULTICAST_TX_MODE :				1
		// INFRASTRUCTURE_UNICAST_TX_MODE : 	2
		// INFRASTRUCTURE_MULTICAST_TX_MODE :	3
       	int preconfiguredTxMode = default(2);
       	
        double period @unit("s") = default(0.2s);
        double startTime @unit("s") = default(0s);
        
    gates:
        
        input meClusterizeAppIn[];
        output meClusterizeAppOut[];
}