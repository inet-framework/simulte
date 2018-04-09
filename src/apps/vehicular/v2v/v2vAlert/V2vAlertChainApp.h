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


#ifndef __SIMULTE_V2VALERTCHAINAPP_H_
#define __SIMULTE_V2VALERTCHAINAPP_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/vehicular/v2v/v2vAlert/packets/V2vAlertPacket_m.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"

using namespace omnetpp;


/**
 * V2vAlertSender
 *
 *  The task of this class is:
 *       1) Sending according the txMode the V2vAlertPacket to the v2vPeerAddresses
 */


class V2vAlertChainApp : public cSimpleModule
{
    inet::UDPSocket socket;

    int nextSno_;
    int size_;
    simtime_t period_;

    cMessage *selfSender_;

    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    std::string carSymbolicAddress;

    cModule* lteNic;

    int clusterID;
    int txMode;
    std::string v2vPeerList;

    simsignal_t v2vAlertChainSentMsg_;

    simsignal_t v2vAlertChainDelay_;
    simsignal_t v2vAlertChainRcvdMsg_;

    public:

        ~V2vAlertChainApp();
        V2vAlertChainApp();

        //allowing the UEClusterizeApp instance connected with this instance
        //to set the computed cluster configuration!
        void setClusterConfiguration(ClusterizeConfigPacket* pkt);

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        //
        // send the alertPacket
        void sendV2vAlertPacket(V2vAlertPacket* pkt);
        //
        // emits statistics and propagates the alertPacket
        void handleV2vAlertPacket(V2vAlertPacket* pkt);

        // utility:
        //          split a string by the delimiter chars
        std::vector<std::string> splitString(std::string, const char* delimiter);
};

#endif
