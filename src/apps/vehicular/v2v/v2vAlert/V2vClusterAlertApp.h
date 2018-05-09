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


#ifndef __SIMULTE_V2VCLUSTERALERTAPP_H_
#define __SIMULTE_V2VCLUSTERALERTAPP_H_

#include <omnetpp.h>

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/vehicular/v2v/v2vAlert/packets/V2vAlertPacket_m.h"

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"

//extending the V2v Cluster Base App
#include "../../v2v/V2vClusterBaseApp.h"

using namespace omnetpp;

/**
 * V2vClusterAlertApp extending V2vClusterBaseApp that allows the ClusterConfiguration by associated UEClusterizeApp
 *
 *  The task of this class is:
 *       1) if the car is the cluster leader: Sending the V2vAlertPacket, according to the txMode, to the v2vReceivers
 *       2) if the car is a cluster member: Forwarding the V2vAlertPacket received, according to the txMode, to the v2vReceivers
 */

class V2vClusterAlertApp : public V2vClusterBaseApp
{
    simsignal_t v2vClusterAlertSentMsg_;
    simsignal_t v2vClusterAlertDelay_;
    simsignal_t v2vClusterAlertRcvdMsg_;

    public:

        ~V2vClusterAlertApp();
        V2vClusterAlertApp();

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

        //
        //
        void sendV2vUnicast(V2vAlertPacket* pkt);

        //
        //
        void sendV2vMulticast(V2vAlertPacket* pkt);         //TODO

        //
        //
        void sendLteUnicast(V2vAlertPacket* pkt);

        //
        //
        void sendLteMulticast(V2vAlertPacket* pkt);         //TODO


        //utility
        //
        bool sendSocketPacket(int sqn, simtime_t time, int size, std::string source, std::string forwarder, std::string destination);
};

#endif
