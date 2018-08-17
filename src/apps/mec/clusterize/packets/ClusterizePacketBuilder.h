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


#ifndef APPS_VEHICULAR_MEC_CLUSTERIZE_PACKETS_CLUSTERIZEPACKETBUILDER_H_
#define APPS_VEHICULAR_MEC_CLUSTERIZE_PACKETS_CLUSTERIZEPACKETBUILDER_H_

#include <omnetpp.h>

#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"

#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_Types.h"

class ClusterizePacketBuilder {
    public:
        ClusterizePacketBuilder();
        virtual ~ClusterizePacketBuilder();

        ClusterizePacket* buildClusterizePacket(const char* type, unsigned int sqn, simtime_t time, long int size, int carOmnetID, const char* srcAddr, const char* destAddr);
        ClusterizeInfoPacket* buildClusterizeInfoPacket(unsigned int sqn, simtime_t time, long int size, int carID, const char* srcAddr, const char* destAddr, inet::Coord position, inet::Coord speed, inet::EulerAngles angularPosition, inet::EulerAngles angularSpeed);
        ClusterizeConfigPacket* buildClusterizeConfigPacket(unsigned int sqn, simtime_t time, unsigned long eventID, int hops, long int size, int carID, const char* srcAddr, const char* destAddr, int clusterID, const char* clusterColor, const char* txMode,  const char* following, const char* follower, const char* clusterString, std::vector<double> accelerations);

        // utility:
        //          split a string by the delimiter chars
        std::vector<std::string> splitString(std::string, const char* delimiter);
};

#endif /* APPS_VEHICULAR_MEC_CLUSTERIZE_PACKETS_CLUSTERIZEPACKETBUILDER_H_ */
