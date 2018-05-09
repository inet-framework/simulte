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
#include "inet/common/geometry/common/EulerAngles.h"


/**
 * MEClusterizeService
 *
 */

struct cluster{

    int id;
    std::string color;

    std::vector<int> members;          //array of cars belonging the cluster: key in cars (map)
    std::string membersList;
};

struct car{

    int id;
    std::string simbolicAddress;

    //local-info
    inet::Coord position;
    inet::Coord speed;
    inet::EulerAngles angularPosition;
    inet::EulerAngles angularSpeed;

    //cluster-info
    int clusterID;
    std::string following;
    std::string follower;
    int txMode;

    //flags/control
    bool isFollower;
    int followingKey = -1;
    int followerKey = -1;
};

class MEClusterizeService : public cSimpleModule
{
protected:
        cMessage *selfSender_;
        simtime_t period_;

        cModule* mePlatform;
        cModule* meHost;

        int maxMEApps;
        std::vector<std::string> colors;            //Cluster Colors

        // for each MEClusterizeApp (linked to the UEClusterizeApp & V2VApp supported)
        // storing the more recent car-local info (from  ClusterizeInfoPacket )
        // and cluster-info computed
        //
        std::map<int, car> cars;                             //i.e. key = gateIndex to the MEClusterizeApp

        // for each Clusters computed
        // storing its informations
        //
        std::map<int, cluster> clusters;

    public:

        ~MEClusterizeService();
        MEClusterizeService();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        // executing periodically the clustering algorithm & updating the v2vConfig map
        // chain clustering alforithm:
        //      building up a chain of cars -> each car is able to send to the car behind!
        //      using the proximityTreshold and directionDelimiterThreshold
        virtual void compute(){};

        // sending, after compute(), the configurations in v2vConfig map
        virtual void sendConfig();

        // updating the v2vInfo map
        // by modifying the values in the correspondent v2vInfo entry (arrival-gate index)
        virtual void handleClusterizeInfo(ClusterizeInfoPacket*);

        // updating the v2vInfo & v2vConfig maps
        // by erasing the correspondent v2vInfo entry (arrival-gate index)
        virtual void handleClusterizeStop(ClusterizePacket*);
};

#endif