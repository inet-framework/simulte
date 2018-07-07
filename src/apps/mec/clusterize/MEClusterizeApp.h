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


#ifndef __SIMULTE_MECLUSTERIZEAPP_H_
#define __SIMULTE_MECLUSTERIZEAPP_H_

#include <omnetpp.h>

#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/mec/clusterize/packets/ClusterizePacketBuilder.h"


/**
 * MEClusterizeApp
 *
 *      1) Forwarding INFO_CLUSTERIZE (INFO_UEAPP) ClusterizeInfoPacket to the MEClusterizeService
 *      2) Receiving CONFIG_CLUSTERIZE (INFO_MEAPP) ClusterizeConfigPacket from theMEClusterizeService
 *          updating with UEClusterizeApp specific information and forwarding to VirtualisationManager
 *      3) Sending STOP_CLUSTERIZE (STOP_MEAPP) ClusterizePacket to the MEClusterizeService when calling finish()
 */

class MEClusterizeApp : public cSimpleModule
{
    int nextSnoConfig_;
    int size_;

    char* ueSimbolicAddress;
    char* meHostSimbolicAddress;
    inet::L3Address destAddress_;


    simsignal_t clusterizeConfigSentMsg_;
    simsignal_t clusterizeInfoRcvdMsg_;
    simsignal_t clusterizeInfoDelay_;

    public:

        ~MEClusterizeApp();
        MEClusterizeApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        // sending a STOP_CLUSTERIZE (STOP_MEAPP) ClusterizePacket to the MEClusterizeService
        // to trigger to erase the correspondent map entries
        void finish();

        // handling INFO_CLUSTERIZE (INFO_UEAPP) ClusterizeInfoPacket from the UEClusterizeApp
        // by forwarding to the MEClusterizeService
        void handleClusterizeInfo(ClusterizeInfoPacket *);

        // handling CONFIG_CLUSTERIZE (INFO_MEAPP) ClusterizeConfigPacket from the MEClusterizeService
        // by adding UEClusterizeApp related info and forwarding to the VirtualisationManager
        void handleClusterizeConfig(ClusterizeConfigPacket *);
};

#endif
