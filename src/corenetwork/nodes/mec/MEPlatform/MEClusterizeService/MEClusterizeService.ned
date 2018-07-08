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
//#	MEClusterizeService
//
//	Simple Module for a ME Service communicating with a ME App on a ME HOST.
//	The task of this module is:
//								1)

simple MEClusterizeService like IMEService
{
    parameters:
        //auto-scheduling informations
        double startTime @unit("s") = default(0s);
        double period @unit("s") = default(0.2s);
        
        //txMode information: how to send the INFO_MEAPP ClusterizeConfigPacket to the UEClusterizeApp: from ClusterizePacket_types.h
        // V2V_UNICAST_TX_MODE :				"V2V_UNICAST"
		// V2V_MULTICAST_TX_MODE :				"V2V_MULTICAST"
		// DOWNLINK_UNICAST_TX_MODE : 			"DOWNLINK_UNICAST"
		// DOWNLINK_MULTICAST_TX_MODE :			"DOWNLINK_MULTICAST"			not supported by Lte Nic
		// HYBRID_TX_MODE:						"HYBRID"						not yet implemented in MEClusterizeService.cpp
       	string preconfiguredTxMode = default("DOWNLINK_UNICAST");
       	
    gates:
        
        input meAppIn[];
        output meAppOut[];     
}