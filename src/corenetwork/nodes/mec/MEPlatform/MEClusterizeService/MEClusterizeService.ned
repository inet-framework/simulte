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
        
        //# mode how to send the cluster configuration to the UEClusterizeApp: from ClusterizePacket_types.h
        // V2V_UNICAST_TX_MODE :				"V2V_UNICAST"
		// V2V_MULTICAST_TX_MODE :				"V2V_MULTICAST"
		// DOWNLINK_UNICAST_TX_MODE : 			"DOWNLINK_UNICAST"
		// DOWNLINK_MULTICAST_TX_MODE :			"DOWNLINK_MULTICAST"
		// HYBRID_TX_MODE:						"HYBRID"
       	string preconfiguredTxMode = default("DOWNLINK_UNICAST");
       	
       	
        double period @unit("s") = default(0.2s);
        double startTime @unit("s") = default(0s);
        
    gates:
        
        input meAppIn[];
        output meAppOut[];     
}