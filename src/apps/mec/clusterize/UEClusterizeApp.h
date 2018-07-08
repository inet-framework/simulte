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

//UDP SOCKET for INET COMMUNICATION WITH ME HOST
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//INET MOBILITY to get position, speed and direction  (and set acceleration)
#include "inet/common/geometry/common/Coord.h"
#include "inet/common/geometry/common/EulerAngles.h"
#include "inet/mobility/contract/IMobility.h"
#include "inet/mobility/single/LinearMobility.h"

//VEINS MOBILITY to get position, speed and direction (and set acceleration)    !TO REVIEW!
#include "veins_inet/VeinsInetMobility.h"
//#include "veins_inet/VeinsInetManager.h"
//#include "veins/modules/mobility/traci/TraCICommandInterface.h"

//APPLICATION SPECIFIC PACKETS and UTILITIES
#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/mec/clusterize/packets/ClusterizePacketBuilder.h"

//EMITTING V2V MULTIHOP STATISTICS
#include "apps/d2dMultihop/statistics/MultihopD2DStatistics.h"

/*
 * UEClusterizeApp see UEClusterizeApp.ned
 */

class UEClusterizeApp : public cSimpleModule
{
    //--------------------------------------
    //auto-scheduling
    cMessage *selfStart_;
    cMessage *selfSender_;
    cMessage *selfStop_;
    simtime_t period_;
    bool stopped;   //to ignore INFO_MEAPP ClusterizeConfigPacket received in V2V txMode after stopping
    //--------------------------------------
    //packet informations
    int nextSnoStart_;
    int nextSnoInfo_;
    int nextSnoStop_;
    int size_;
    //--------------------------------------
    //communication with ME Host
    inet::UDPSocket socket;
    inet::L3Address destAddress_;
    int localPort_;
    int destPort_;
    //symbolic names
    char* mySymbolicAddress;            //i.e. Car[x]
    char* meHostSymbolicAddress;        //i.e. meHost.virtualisationInfrastructure
    //--------------------------------------
    //multicast communication with cars
    inet::UDPSocket multicastSocket;
    inet::L3Address multicastAddress_;
    int multicastPort_;
    char* multicastGoupAddress;
    //--------------------------------------
    //clustering informations
    int cluserID;
    double requiredAcceleration;            //acceleration received by MEClusterizeService (used for mobility)
    char* carFollowerSymbolicAddress;       //symbolic car follower name (i.e. Car[y])
    inet::L3Address carFollowerAddress_;
    char* txMode;
    //--------------------------------------
    //mobility informations
    cModule* car;
    //inet
    inet::LinearMobility *linear_mobility;      //to retrieve mobility info (and set acceleration)
    inet::Coord position;
    inet::Coord speed;
    inet::EulerAngles angularPosition;
    inet::EulerAngles angularSpeed;
    double maxSpeed;
    //veins
    Veins::VeinsInetMobility *veins_mobility;   //to retrieve mobility info
    std::string carVeinsID;
    //Veins::VeinsInetManager *veinsManager;                    //to retrieve TraCICommandInterface
    //Veins::TraCICommandInterface* traci;                      //to retrieve TraCICommandInterface::Vehicle
    //Veins::TraCICommandInterface::Vehicle *traciVehicle;      //to access the slowDown() method to set mobility acceleration
    //--------------------------------------
    //collecting multihop statistics
    MultihopD2DStatistics* stat_;
    cModule* lteNic;
    MacNodeId macID;
    //--------------------------------------
    //collecting application statistics
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
        //----------------------------------------------------------------------------------------------------------------------
        // sending, until receiving an ACK_START_MEAPP, to the meHost (VirtualisationManager) a START_MEAPP ClusterizePacket
        // requesting to instantiate a MEClusterizeApp
        void sendClusterizeStartPacket();
        // handling ACK_START_MEAPP ClusterizePacket
        // by stopping the sendClusterizeStartPacket() and starting sendClusterizeInfoPacket()
        void handleMEAppAckStart(MEAppPacket*);
        //----------------------------------------------------------------------------------------------------------------------
        // sending to the correspondent MEClusterizeApp (through VirtualisationManager) an INFO_MEAPP ClsuterizeInfoPacket
        // to communicate car-local informations (i.e. position, speed)
        void sendClusterizeInfoPacket();
        //----------------------------------------------------------------------------------------------------------------------
        // sending, until receiving an ACK_STOP_MEAPP, to the meHost (VirtualisationManager) a STOP_MEAPP ClusterizePacket
        // requesting to terminate the correspondent MEClusterizeApp
        void sendClusterizeStopPacket();
        // handling ACK_STOP_MEAPP ClusterizePacket
        // by stopping sendClusterizeStopPacket()
        void handleMEAppAckStop(MEAppPacket*);
        //----------------------------------------------------------------------------------------------------------------------
        // handling INFO_MEAPP ClusterizeConfigPacket
        // by calling handleClusterizeConfigFromMEHost or handleClusterizeConfigFromUE and emit statistics
        void handleClusterizeConfig(ClusterizeConfigPacket *);
        // handling INFO_MEAPP ClusterizeConfigPacket from ME Host:
        // eventually propagating (if LEADER) in V2V UNICAST or V2V MULTICAST to the follower car
        void handleClusterizeConfigFromMEHost(ClusterizeConfigPacket *);
        // handling INFO_MEAPP ClusterizeConfigPacket from UE (car):
        // retrieving following and follower UE (car)
        // eventually propagating in V2V UNICAST to the follower car
        void handleClusterizeConfigFromUE(ClusterizeConfigPacket *);
        //----------------------------------------------------------------------------------------------------------------------
        //utilities functions
        //retrieve the follower car from the INFO_MEAPP ClusterizeConfigPacket
        std::string getFollower(ClusterizeConfigPacket *);
        //retrieve the previous car from the INFO_MEAPP ClusterizeConfigPacket
        std::string getFollowing(ClusterizeConfigPacket *);
        //retrieve the acceleration to use from the INFO_MEAPP ClusterizeConfigPacket
        double updateAcceleration(ClusterizeConfigPacket *);
        //----------------------------------------------------------------------------------------------------------------------
        //getting veins module to handle vehicle mobility
        void getVehicleInterface();
};

#endif
