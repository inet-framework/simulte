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

//SIMULTE BINDER (Oracle Module) and UTILITIES
#include "corenetwork/binder/LteBinder.h"
#include "common/LteCommon.h"

//INET GEOMETRY to MANIPULATE position, speed and direction
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"

//APPLICATION SPECIFIC PACKETS and UTILITIES
#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/mec/clusterize/packets/ClusterizePacketBuilder.h"

//EMITTING V2V MULTIHOP STATISTICS
#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"

//################################################################################################
//DATA STRUCTURE TYPES
struct cluster{
    int id;                             //cluster ID
    std::string color;                  //color assigned to the cluster (set in ClusterizeConfigPacket)
    std::string membersList;            //Symbolic UE Addresses space-separated  (set in ClusterizeConfigPacket)
    std::vector<double> accelerations;  //ordered-array containing the accelerations for each Cluster Member  (set in ClusterizeConfigPacket)
    std::vector<int> members;           //ordered-array containing the key (in cars map) for each Cluster Member
    //updating platoon formation info
    std::vector<double> distancies;       //w.r.t  previous vehicle
};
struct car{

    int id;
    std::string symbolicAddress;        //i.e. car[x]
    MacNodeId macID;                    //used for rni-service
    //----------------------------------
    //local-info                        //updated in handleClusterizeInfo()
    simtime_t timestamp;
    inet::Coord position;
    inet::Coord speed;
    inet::EulerAngles angularPosition;
    inet::EulerAngles angularSpeed;
    double acceleration = 0;
    //----------------------------------
    //rni info
    double txPower;
    Cqi cqi;
    //----------------------------------
    //cluster-info                      //updated in compute()
    int clusterID;
    std::string following;
    std::string follower;
    std::string txMode;
    //----------------------------------
    //flags/control                     //used in compute()
    bool isLeader;
    bool isFollower;
    int followingKey = -1;
    int followerKey = -1;
};
//################################################################################################

/**
 * MEClusterizeService See MEClusterizeService.ned
 *
 */
class MEClusterizeService : public cSimpleModule
{
    protected:
        //----------------------------------
        //auto-scheduling
        cMessage *selfSender_;
        simtime_t period_;
        //----------------------------------
        //parent modules
        cModule* mePlatform;
        cModule* meHost;
        //freshness informations
        simtime_t lastRun;                              //discarding car-updates too old!
        //----------------------------------
        //txMode information for sending INFO_MEAPP
        std::string preconfiguredTxMode;                //supported: INFRASTRUCTURE_UNICAST - V2V_UNICAST - V2V_MULTICAST
        //----------------------------------
        //data structures
        //informations about cars
        std::map<int, car> cars;                        //i.e. key = gateIndex to the MEClusterizeApp
        //informations about clusters
        std::map<int, cluster> clusters;                //i.e. key = leader-car omnet id
        //colors to assign to clusters
        std::vector<std::string> colors;
        //----------------------------------
        //binder
        LteBinder* binder_;
        //----------------------------------
        //statistics
        MultihopD2DStatistics* stat_;
        unsigned long eventID;

    public:
        ~MEClusterizeService();
        MEClusterizeService();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        //----------------------------------------------------------------------------------------------------------------------
        // executing periodically the clustering algorithm & updating the clusters map
        virtual void compute(){};
        // sending, after compute(), the INFO_MEAPP ClusterizeConfigPacket using the clusters map informations
        virtual void sendConfig();
        //----------------------------------------------------------------------------------------------------------------------
        // handling INFO_UEAPP ClusterizeInfoPacket by updating cars map at the correspondent entry (arrival-gate index)
        virtual void handleClusterizeInfo(ClusterizeInfoPacket*);
        // handling STOP_MEAPP ClusterizePacket by erasing cars map at the correspondent entry (arrival-gate index)
        virtual void handleClusterizeStop(ClusterizePacket*);
};

#endif
