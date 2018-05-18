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



#ifndef __SIMULTE_V2VCLUSTERBASEAPP_H_
#define __SIMULTE_V2VCLUSTERBASEAPP_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"

using namespace omnetpp;

/**
 * V2vClusterBaseApp
 *
 *  The task of this class is:
 *       1) providing a Base Simple Module for implementing V2v Applications running paired with UEClusterizeApp
 *          and being configured as member of a cluster of vehicles

 */
class V2vClusterBaseApp : public cSimpleModule
{
    protected:

        inet::UDPSocket socket;

        int nextSno_;
        int size_;
        simtime_t period_;

        cMessage *selfSender_;

        int localPort_;
        int destPort_;
        inet::L3Address destAddress_;
        std::string follower;

        std::string carSymbolicAddress;

        cModule* lteNic;

        bool clusterLeader;
        int clusterID;
        int txMode;
        std::string v2vReceivers;

    public:

        ~V2vClusterBaseApp();
        V2vClusterBaseApp();

        //allowing the UEClusterizeApp instance connected with this instance
        //to set the computed cluster configuration!
        void setClusterConfiguration(ClusterizeConfigPacket* pkt);

    protected:

        virtual void initialize(int stage);
        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        virtual void handleMessage(cMessage *msg);

        // utility:
        //          split a string by the delimiter chars
        std::vector<std::string> splitString(std::string, const char* delimiter);
};

#endif
