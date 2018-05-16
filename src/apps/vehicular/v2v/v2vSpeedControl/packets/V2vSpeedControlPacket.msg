//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//


//
// Packet used by the V2vSpeedControlApp

packet V2vSpeedControlPacket {
    
    unsigned int sno;
    simtime_t timestamp;
    
    string sourceAddress;
    string destinationAddress;
    
    string forwarderAddress;
    
    double clusterSpeed;
    double clusterDistance;
    double clusterSlowDownTime;	//ms in which to reach the given clusterSpeed
    
    double followingPositionX;
    double followingPositionY;
    double followingPositionZ;    
}
