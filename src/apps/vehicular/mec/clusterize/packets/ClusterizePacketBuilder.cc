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


#include "ClusterizePacketBuilder.h"

ClusterizePacketBuilder::ClusterizePacketBuilder() {
    // TODO Auto-generated constructor stub

}

ClusterizePacketBuilder::~ClusterizePacketBuilder() {
    // TODO Auto-generated destructor stub
}

ClusterizePacket* ClusterizePacketBuilder::buildClusterizePacket(const char* type, unsigned int sqn, simtime_t time, long int size, int carOmnetID, const char* srcAddr, const char* destAddr){

        ClusterizePacket* packet = new ClusterizePacket(type);

        //MEAppPacket info
        packet->setSno(sqn);
        packet->setTimestamp(time);
        packet->setByteLength(size);
        packet->setType(type);
        packet->setSourceAddress(srcAddr);
        packet->setDestinationAddress(destAddr);
        //important to instantiate the MEClusterizeApp by the Virtualisation Manager!
        packet->setMEModuleType( ME_CLUSTERIZE_APP_MODULE_TYPE);
        packet->setMEModuleName(ME_CLUSTERIZE_APP_MODULE_NAME);
        packet->setRequiredService(ME_CLUSTERIZE_SERVICE_MODULE_NAME);

        //ClusterizePacket info
        if(!strcmp(type, START_MEAPP)){
            packet->setName(START_CLUSTERIZE);
        }else
            if(!strcmp(type, STOP_MEAPP))
                packet->setName(STOP_CLUSTERIZE);
        else
            if(!strcmp(type, ACK_START_MEAPP))
                packet->setName(ACK_START_CLUSTERIZE);
        else
            if(!strcmp(type, ACK_STOP_MEAPP))
                packet->setName(ACK_STOP_CLUSTERIZE);

        packet->setCarOmnetID(carOmnetID);

        return packet;
}

ClusterizeInfoPacket* ClusterizePacketBuilder::buildClusterizeInfoPacket(unsigned int sqn, simtime_t time, long int size, int carOmnetID, const char* srcAddr, const char* destAddr, inet::Coord position, inet::Coord speed, inet::EulerAngles angularPosition, inet::EulerAngles angularSpeed){

    ClusterizeInfoPacket* packet = new ClusterizeInfoPacket(INFO_CLUSTERIZE);

    //MEAppPacket info
    packet->setSno(sqn);
    packet->setTimestamp(time);
    packet->setByteLength(size);
    packet->setType(INFO_UEAPP);
    packet->setSourceAddress(srcAddr);
    packet->setDestinationAddress(destAddr);
    //important to instantiate the MEClusterizeApp by the Virtualisation Manager!
    packet->setMEModuleType( ME_CLUSTERIZE_APP_MODULE_TYPE);
    packet->setMEModuleName(ME_CLUSTERIZE_APP_MODULE_NAME);
    packet->setRequiredService(ME_CLUSTERIZE_SERVICE_MODULE_NAME);

    //ClusterizePacket info
    packet->setName(INFO_CLUSTERIZE);
    packet->setCarOmnetID(carOmnetID);

    //ClusterizeInfoPacket info
    packet->setPositionX(position.x);
    packet->setPositionY(position.y);
    packet->setPositionZ(position.z);
    packet->setSpeedX(speed.x);
    packet->setSpeedY(speed.y);
    packet->setSpeedZ(speed.z);
    packet->setAngularPositionA(angularPosition.alpha);
    packet->setAngularPositionB(angularPosition.beta);
    packet->setAngularPositionC(angularPosition.gamma);
    packet->setAngularSpeedA(angularSpeed.alpha);
    packet->setAngularSpeedB(angularSpeed.beta);
    packet->setAngularSpeedC(angularSpeed.gamma);

    return packet;
}

ClusterizeConfigPacket* ClusterizePacketBuilder::buildClusterizeConfigPacket(unsigned int sqn, simtime_t time,  unsigned long eventID, int hops, long int size, int carOmnetID, const char* srcAddr, const char* destAddr, int clusterID, const char* clusterColor, const char* txMode, const char* following, const char* follower, const char* clusterString, std::vector<double> accelerations){

    ClusterizeConfigPacket* packet = new ClusterizeConfigPacket(CONFIG_CLUSTERIZE);

    //MEAppPacket info
    packet->setSno(sqn);
    packet->setTimestamp(time);
    packet->setByteLength(size);
    packet->setType(INFO_MEAPP);
    packet->setSourceAddress(srcAddr);
    packet->setDestinationAddress(destAddr);
    //important to instantiate the MEClusterizeApp by the Virtualisation Manager!
    packet->setMEModuleType( ME_CLUSTERIZE_APP_MODULE_TYPE);
    packet->setMEModuleName(ME_CLUSTERIZE_APP_MODULE_NAME);
    packet->setRequiredService(ME_CLUSTERIZE_SERVICE_MODULE_NAME);

    //ClusterizePacket info
    packet->setName(CONFIG_CLUSTERIZE);
    packet->setCarOmnetID(carOmnetID);

    //ClusterizeConfigPacket info
    packet->setEventID(eventID);
    packet->setHops(hops);
    packet->setClusterID(clusterID);
    packet->setClusterColor(clusterColor);
    packet->setTxMode(txMode);
    packet->setClusterFollowing(following);
    packet->setClusterFollower(follower);
    packet->setClusterString(clusterString);

    //build list of accelerations
    packet->setAccelerationsArraySize(accelerations.size());
    for(int i=0; i < accelerations.size(); i++)
        packet->setAccelerations(i, accelerations.at(i));

    //split clusterString
    std::vector<std::string> clusterVector = splitString(clusterString, " -> ");

    packet->setClusterListArraySize(clusterVector.size());
    for(int i=0; i < clusterVector.size(); i++)
        packet->setClusterList(i, clusterVector.at(i).c_str());

    return packet;
}

/*
 * utility function
 */
std::vector<std::string> ClusterizePacketBuilder::splitString(std::string v2vReceivers, const char* delimiter){

    std::vector<std::string> v2vAddresses;
    char* token;
    char* str = new char[v2vReceivers.length() + 1];
    std:strcpy(str, v2vReceivers.c_str());

    token = strtok (str , delimiter);            // split by spaces

    while (token != NULL)
    {
      v2vAddresses.push_back(token);
      token = strtok (NULL, delimiter);         // split by spaces
    }

    delete(str);

    return v2vAddresses;
}
