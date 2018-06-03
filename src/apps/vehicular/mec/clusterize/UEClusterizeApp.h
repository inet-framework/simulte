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


#ifndef __SIMULTE_UECLUSTERIZEAPP_H_
#define __SIMULTE_UECLUSTERIZEAPP_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "corenetwork/binder/LteBinder.h"

#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"

#include "inet/mobility/contract/IMobility.h"
#include "veins_inet/VeinsInetMobility.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketBuilder.h"

//to emit V2V statistics
#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"

//to get the V2v Cluster App reference and to configure the cluster configuration --> calling setClusterConfiguration cross-module Call
#include "../../v2v/V2vClusterBaseApp.h"


/*
 * UEClusterizeAPP
 *
 *  Sender Side task is:
 *       1) Sending START_CLUSTERIZE ClusterizePacket to the meHost (VirtualisationManager)
 *       2) Sending periodically INFO_CLUSTERIZE ClusterizeInfoPacket to the correspondent MEClusterizeApp
 *
 *  Receiver Side task is:
 *       1) Receiving CONFIG_CLUSTERIZE ClsuterizeConfigPacket from the MEClusterizeApp
 *          and configuring the V2V App, with the received configuration, by calling the V2V App public set method
 */

class UEClusterizeApp : public cSimpleModule
{
    //unicast
    inet::UDPSocket socket;
    int nextSnoStart_;
    int nextSnoInfo_;
    int nextSnoStop_;
    int size_;
    simtime_t period_;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    char* sourceSimbolicAddress;            //Car[x]
    char* destSimbolicAddress;              //meHost.virtualisationInfrastructure
    MacNodeId macID;

    char* v2vFollowerSimbolicAddress;       //Car[y]
    inet::L3Address v2vFollowerAddress_;

    //multicast
    inet::UDPSocket multicastSocket;
    int multicastPort_;
    char* multicastGoupAddress;
    inet::L3Address multicastAddress_;

    cMessage *selfStart_;
    cMessage *selfSender_;
    cMessage *selfStop_;

    // mobility information for MEClusterizeService computations
    //
    cModule* lteNic;
    cModule* car;
    Veins::VeinsInetMobility *veins_mobility;
    inet::IMobility *mobility;
    inet::Coord position;
    inet::Coord speed;
    inet::EulerAngles angularPosition;
    inet::EulerAngles angularSpeed;
    double maxSpeed;

    // reference to the statistics manager
    MultihopD2DStatistics* stat_;

    // v2v app who uses the cluster configuration received from MEClusterizeService (optional usage)
    //
    cModule* v2vApp;                        //v2vApp module
    char* v2vAppName;                   //v2vApp Class Name
    V2vClusterBaseApp *v2vClusterApp;

    //signals
    simsignal_t clusterizeInfoSentMsg_;
    simsignal_t clusterizeConfigRcvdMsg_;
    simsignal_t clusterizeConfigDelay_;
    public:

        ~UEClusterizeApp();
        UEClusterizeApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        virtual void finish();

        // sending, until receiving an ack, to the meHost (VirtualisationManager) a START_CLUSTERIZE ClusterizePacket
        // requesting to instantiate a MEClusterizeApp
        void sendClusterizeStartPacket();

        // handling ACK_START_CLUSTERIZE ClusterizePacket
        // by stopping the sendClusterizeStartPacket() and starting sendClusterizeInfoPacket()
        void handleClusterizeAckStart(ClusterizePacket*);

        // sending to the correspondent MEClusterizeApp (trhough VirtualisationManager) an INFO_CLUSTERIZE ClsuterizeInfoPacket
        // to communicate car-local informations (i.e. position, speed)
        void sendClusterizeInfoPacket();

        // sending, until receiving an ack, to the meHost (VirtualisationManager) a STOP_CLUSTERIZE ClusterizePacket
        // requesting to terminate the correspondent MEClusterizeApp
        void sendClusterizeStopPacket();

        // handling ACK_STOP_CLUSTERIZE ClusterizePacket
        // by stopping sendClusterizeStopPacket()
        void handleClusterizeAckStop(ClusterizePacket*);

        // handling CONFIG_CLUSTERIZE ClusterizeConfigPacket
        // by calling handleClusterizeConfigFromMEHost or handleClusterizeConfigFromUE and emitting statistics
        void handleClusterizeConfig(ClusterizeConfigPacket *);

        // handling CONFIG_CLUSTERIZE ClusterizeConfigPacket from ME Host:
        // setting info on the V2V ClusterBase App
        // eventually propagating in V2V UNICAST or V2V MULTICAST
        void handleClusterizeConfigFromMEHost(ClusterizeConfigPacket *);

        // handling CONFIG_CLUSTERIZE ClusterizeConfigPacket from UE (car):
        // retrieving following and follower UE (car)
        // setting info on the V2V ClusterBase App
        // eventually propagating in V2V UNICAST
        void handleClusterizeConfigFromUE(ClusterizeConfigPacket *);

        std::string getFollower(ClusterizeConfigPacket *);
        std::string getFollowing(ClusterizeConfigPacket *);
};

#endif
