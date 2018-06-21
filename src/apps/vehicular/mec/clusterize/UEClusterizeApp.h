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

//inet mobility
#include "inet/mobility/contract/IMobility.h"
#include "inet/mobility/single/LinearMobility.h"

//veins mobility
#include "veins_inet/VeinsInetMobility.h"
//#include "veins_inet/VeinsInetManager.h"
//#include "veins/modules/mobility/traci/TraCICommandInterface.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketBuilder.h"

//to emit V2V statistics
#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"


/*
 * UEClusterizeAPP
 *
 *  Sender Side task is:
 *       1) Sending START_CLUSTERIZE ClusterizePacket to the meHost (VirtualisationManager)
 *       2) Sending periodically INFO_CLUSTERIZE ClusterizeInfoPacket to the correspondent MEClusterizeApp
 *
 *  Receiver Side task is:
 *       1) Receiving CONFIG_CLUSTERIZE ClsuterizeConfigPacket from the MEClusterizeApp
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

    //cluster info
    int cluserID;
    char* v2vFollowerSimbolicAddress;       //Car[y]
    inet::L3Address v2vFollowerAddress_;
    double requiredAcceleration;


    //multicast
    inet::UDPSocket multicastSocket;
    int multicastPort_;
    char* multicastGoupAddress;
    inet::L3Address multicastAddress_;

    // mobility information for MEClusterizeService computations
    //
    cModule* lteNic;
    cModule* car;
    //veins
    Veins::VeinsInetMobility *veins_mobility;
    std::string carVeinsID;
    //Veins::VeinsInetManager *veinsManager;
    //Veins::TraCICommandInterface* traci;
    //Veins::TraCICommandInterface::Vehicle *traciVehicle;
    //inet
    inet::IMobility *mobility;

    //info
    inet::Coord position;
    inet::Coord speed;
    inet::EulerAngles angularPosition;
    inet::EulerAngles angularSpeed;
    double maxSpeed;


    //scheduling
    cMessage *selfStart_;
    cMessage *selfSender_;
    cMessage *selfStop_;

    // reference to the statistics manager
    MultihopD2DStatistics* stat_;

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
        void handleMEAppAckStart(MEAppPacket*);

        // sending to the correspondent MEClusterizeApp (trhough VirtualisationManager) an INFO_CLUSTERIZE ClsuterizeInfoPacket
        // to communicate car-local informations (i.e. position, speed)
        void sendClusterizeInfoPacket();

        // sending, until receiving an ack, to the meHost (VirtualisationManager) a STOP_CLUSTERIZE ClusterizePacket
        // requesting to terminate the correspondent MEClusterizeApp
        void sendClusterizeStopPacket();

        // handling ACK_STOP_CLUSTERIZE ClusterizePacket
        // by stopping sendClusterizeStopPacket()
        void handleMEAppAckStop(MEAppPacket*);

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
        double updateAcceleration(ClusterizeConfigPacket *);

        void getVehicleInterface();
};

#endif
