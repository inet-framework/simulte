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

#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"

#include "corenetwork/nodes/mec/MEPlatform/MEClusterizeService/MEClusterizeService.h"

/**
 * MEClusterizeApp
 *
 *      1) Forwarding INFO_CLUSTERIZE ClusterizeInfoPacket to the MEClusterizeService
 *      2) Receiving CONFIG_CLUSTERIZE ClusterizeConfigPacket from theMEClusterizeService
 *          updating with UEClusterizeApp specific information and forwarding to VirtualisationManager
 *      3) Sending STOP_CLUSTERIZE ClusterizePacket to the MEClusterizeService when calling finish()
 */

class MEClusterizeApp : public cSimpleModule
{
    int nextSnoConfig_;
    int size_;
    //simtime_t period_;

    char* sourceSimbolicAddress;
    char* destSimbolicAddress;
    inet::L3Address destAddress_;

    const char* v2vAppName;     //to distinguish the instances correspondent to the same Car

    simsignal_t clusterizeConfigSentMsg_;
    simsignal_t clusterizeInfoRcvdMsg_;

    public:

        ~MEClusterizeApp();
        MEClusterizeApp();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        // sending a STOP_CLUSTERIZE ClusterizePacket to the MEClusterizeService
        // to trigger to erase the correspondent map entries
        void finish();

        // handling INFO_CLUSTERIZE ClusterizeInfoPacket from the UEClusterizeApp
        // by forwarding to the MEClusterizeService
        void handleClusterizeInfo(ClusterizeInfoPacket *);

        // handling CONFIG_CLUSTERIZE ClusterizeConfigPacket from the MEClusterizeService
        // by adding UEClusterizeApp related info and forwarding to the VirtualisationManager
        void handleClusterizeConfig(ClusterizeConfigPacket *);

};

#endif
