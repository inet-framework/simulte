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

cplusplus {{
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_m.h"
}}
packet MEAppPacket;

//
// Basic Clusterize Packet used by the UEClusterizeApp and MEClusterizeApp
//			
// and extensions:
//					ClusterizeInfo
//					ClusterizeCommand

packet ClusterizePacket extends MEAppPacket{
    
    int carOmnetID;	
}

//
// used by MEClusterizeApp to communicate computed-configuration to the UEClusterizeApp
//

packet ClusterizeConfigPacket extends ClusterizePacket{
    
    // cluster identification
    int clusterID;
    string clusterColor;
    string clusterString;
    string clusterList[];
    //cluster formation
    double accelerations[];
    //cluster communication strategy
    string clusterFollowing;
    string clusterFollower;
    string txMode;    
    //statistic informations
    double distancies[];	//distance w.r.t. previous vehicle
    int hops;
    unsigned long eventID;
}

//
// used by UEClusterizeApp to communicate local-information to the MEClusterizeApp
//

packet ClusterizeInfoPacket extends ClusterizePacket{

	// car-local informations    
    double positionX;
    double positionY;
    double positionZ;
    double speedX;
    double speedY;
    double speedZ;
    double acceleration;
    double angularPositionA;
    double angularPositionB;
    double angularPositionC;
    double angularSpeedA;
    double angularSpeedB;
    double angularSpeedC;
}
