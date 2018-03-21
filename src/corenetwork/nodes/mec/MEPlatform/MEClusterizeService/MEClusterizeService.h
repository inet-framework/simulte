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


#ifndef __SIMULTE_MECLUSTERIZESERVICE_H_
#define __SIMULTE_MECLUSTERIZESERVICE_H_

#include <omnetpp.h>

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"

#include "inet/common/geometry/common/Coord.h"

/**
 * MEClusterizeService
 *
 */

struct ueLocalInfo{

    std::string carSimbolicAddress;
    inet::Coord position;
    inet::Coord speed;
};

struct ueClusterConfig{

    std::string carSimbolicAddress;
    int clusterID;
    std::string v2vLocalReceivers;
    int txMode;
};

class MEClusterizeService : public cSimpleModule
{
    cMessage *selfSender_;
    simtime_t period_;

    cModule* mePlatform;
    cModule* meHost;

    int maxMEApps;

    //for each MEClusterizeApp (linked to the UEClusterizeApp & V2VApp supported)
    //storing the more recent ClusterizeInfoPacket containing the car-local info
    //
    std::map<int, ueLocalInfo> v2vInfo;                             //i.e. key = gateIndex to the MEClusterizeApp

    //for each MEClusterizeApp (linked to the UEClusterizeApp & V2VApp supported)
    //storing the newest ClusterizeConfigPacket to send to the UEClusterizeApp
    //
    std::map<int, ueClusterConfig> v2vConfig;                       //i.e. key = gateIndex to the MEClusterizeApp


    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        // executing periodically the clustering algorithm & updating the v2vConfig map
        void compute();

        // sending, after compute(), the configurations in v2vConfig map
        void sendConfig();

        // updating the v2vInfo map
        // by modifying the values in the correspondent v2vInfo entry (arrival-gate index)
        void handleClusterizeInfo(ClusterizeInfoPacket*);

        // updating the v2vInfo & v2vConfig maps
        // by erasing the correspondent v2vInfo entry (arrival-gate index)
        void handleClusterizeStop(ClusterizePacket*);
};

#endif
