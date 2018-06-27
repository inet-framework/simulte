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

#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/mec/clusterize/packets/ClusterizePacketBuilder.h"

#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"

#include "corenetwork/binder/LteBinder.h"
#include "common/LteCommon.h"

//to build V2V statistics records to emit in UEClusterizeApp
#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"


/**
 * MEClusterizeService
 *
 */

struct cluster{

    int id;
    std::string color;

    std::vector<int> members;          //array of cars belonging the cluster: key in cars (map)
    std::string membersList;

    std::vector<double> accelerations;
};

struct car{

    int id;
    std::string simbolicAddress;
    MacNodeId macID;

    //local-info
    inet::Coord position;
    inet::Coord speed;
    inet::EulerAngles angularPosition;
    inet::EulerAngles angularSpeed;

    //rni info
    double txPower;
    Cqi cqi;

    //cluster-info
    int clusterID;
    std::string following;
    std::string follower;

    std::string txMode;

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

        std::string preconfiguredTxMode;                    //from INI: chosing INFRASTRUCTURE_UNICAST - V2V_UNICAST - V2V_MULTICAST

        // for each MEClusterizeApp (linked to the UEClusterizeApp & V2VApp supported)
        // storing the more recent car-local info (from  ClusterizeInfoPacket )
        // and cluster-info computed
        //
        std::map<int, car> cars;                             //i.e. key = gateIndex to the MEClusterizeApp

        // for each Clusters computed
        // storing its informations
        //
        std::map<int, cluster> clusters;


        LteBinder* binder_;

        // reference to the statistics manager
        MultihopD2DStatistics* stat_;
        unsigned long eventID;

    public:

        ~MEClusterizeService();
        MEClusterizeService();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        // executing periodically the clustering algorithm & updating the vclusters map
        virtual void compute(){};

        // sending, after compute(), the configurations in clusters map
        virtual void sendConfig();

        // updating the cars map
        // by modifying the values in the correspondent entry (arrival-gate index)
        virtual void handleClusterizeInfo(ClusterizeInfoPacket*);

        // updating the cars maps
        // by erasing the correspondent entry (arrival-gate index)
        virtual void handleClusterizeStop(ClusterizePacket*);
};

#endif
