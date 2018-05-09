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



#ifndef __SIMULTE_V2VCLUSTERSPEEDCONTROLAPP_H_
#define __SIMULTE_V2VCLUSTERSPEEDCONTROLAPP_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "veins_inet/VeinsInetMobility.h"
#include "veins_inet/VeinsInetManager.h"
#include "veins/modules/mobility/traci/TraCICommandInterface.h"
#include "veins/modules/mobility/traci/TraCIColor.h"

#include "apps/vehicular/v2v/v2vSpeedControl/packets/V2vSpeedControlPacket_m.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"

//extending the V2v Cluster Base App
#include "../../v2v/V2vClusterBaseApp.h"

using namespace omnetpp;

/**
 * V2vClusterSpeedControlApp extending V2vClusterBaseApp that allows the ClusterConfiguration by associated UEClusterizeApp
 *
 *  The task of this class is:
 *       1) if the car is the cluster leader: Sending the V2vSpeedControlPacket, according to the txMode, to the v2vReceivers
 *       2) if the car is a cluster member: Forwarding the V2vSpeedControlPacket received, according to the txMode, to the v2vReceivers
 */

class V2vClusterSpeedControlApp : public V2vClusterBaseApp
{

    //VEINS MOBILITY
    cModule* car;
    std::string carID;
    Veins::VeinsInetMobility *mobility;
    Veins::VeinsInetManager *veinsManager;
    Veins::TraCICommandInterface* traci;
    Veins::TraCICommandInterface::Vehicle *traciVehicle;

    simsignal_t v2vClusterSpeedControlSentMsg_;
    simsignal_t v2vClusterSpeedControlDelay_;
    simsignal_t v2vClusterSpeedControlRcvdMsg_;

    public:

        ~V2vClusterSpeedControlApp();
        V2vClusterSpeedControlApp();

    protected:

        virtual void initialize(int stage);
        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void handleMessage(cMessage *msg);

        //
        // send the alertPacket
        void sendV2vSpeedControlPacket(V2vSpeedControlPacket* pkt);
        //
        // emits statistics and propagates the alertPacket
        void handleV2vSpeedControlPacket(V2vSpeedControlPacket* pkt);

        //
        //
        void getVehicleInterface();
};

#endif
