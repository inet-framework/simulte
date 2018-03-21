//
//                           SimuLTE
//
// This file is part of a software released under the license included in file
// "license.pdf". This license can be also found at http://www.ltesimulator.com/
// The above file and the present reference are part of the software itself,
// and cannot be removed from it.
//

#ifndef __SIMULTE_UECLUSTERIZESENDER_H_
#define __SIMULTE_UECLUSTERIZESENDER_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/vehicular/v2v/v2vAlert/packets/V2vAlertPacket_m.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"

/**
 * V2vAlertSender
 *
 *  The task of this class is:
 *       1) Sending according the txMode the V2vAlertPacket to the v2vPeerAddresses
 */

class V2vAlertSender : public cSimpleModule
{
        inet::UDPSocket socket;

        int nextSno_;
        int size_;
        simtime_t period_;

        simsignal_t v2vAlertSentMsg_;

        cMessage *selfSender_;

        int localPort_;
        int destPort_;
        inet::L3Address destAddress_;

        cModule* lteNic;

        int clusterID;
        int txMode;
        std::string v2vPeerList;

        void sendV2vAlertPacket();

    public:

        ~V2vAlertSender();
        V2vAlertSender();

        //allowing the UEClusterizeApp instance connected with this instance
        //to set the computed cluster configuration!
        void setClusterConfiguration(ClusterizeConfigPacket* pkt);

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        // split a string by the delimiter chars
        std::vector<std::string> splitString(std::string, const char* delimiter);
};

#endif
