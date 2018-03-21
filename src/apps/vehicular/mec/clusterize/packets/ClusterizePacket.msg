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

//
// Basic Clusterize Packet used by the UEClusterizeApp and MEClusterizeApp
//
//	type field is used to distinguish:
//
//			ClusterizeStart
//			ClusterizeAck
//			ClusterizeStop
//			
// and extensions:
//
//			ClusterizeInfo
//			ClusterizeCommand

packet ClusterizePacket {
    unsigned int sno;
    simtime_t timestamp;
    
    string type;
    
    string sourceAddress;
    string destinationAddress;
    
    string v2vAppName;
}

//
// used by MEClusterizeApp to communicate computed-configuration to the UEClusterizeApp
//

packet ClusterizeConfigPacket extends ClusterizePacket{
    
    int clusterID;
    int txMode;
    string v2vPeerList;	// destAddresses for v2vSupportedSender
}


//
// used by UEClusterizeApp to communicate local-information to the MEClusterizeApp
//

packet ClusterizeInfoPacket extends ClusterizePacket{
    
    double positionX;
    double positionY;
    double speedX;
    double speedY;
}
