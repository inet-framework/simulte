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
#define __SIMULTE_MECLUSTERIZEAPP_H

//INET COMMUNICATION WITH UEClusterizeApp
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//APPLICATION SPECIFIC PACKETS and UTILITIES
#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/mec/clusterize/packets/ClusterizePacketBuilder.h"

/**
 * MEClusterizeApp see MEClusterize.ned
 */

class MEClusterizeApp : public cSimpleModule
{
    //--------------------------------------
    //packet informations
    int nextSnoConfig_;
    int size_;
    //--------------------------------------
    //communication with ME Host
    char* ueSimbolicAddress;
    char* meHostSimbolicAddress;
    inet::L3Address destAddress_;
    //--------------------------------------
    //statistics
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
        //----------------------------------------------------------------------------------------------------------------------
        // sending a STOP_MEAPP ClusterizePacket to the MEClusterizeService
        // to trigger to erase the correspondent map entries
        void finish();
        //----------------------------------------------------------------------------------------------------------------------
        // handling INFO_UEAPP ClusterizeInfoPacket from the UEClusterizeApp
        // by forwarding to the MEClusterizeService
        void handleClusterizeInfo(ClusterizeInfoPacket *);
        //----------------------------------------------------------------------------------------------------------------------
        // handling INFO_MEAPP ClusterizeConfigPacket from the MEClusterizeService
        // by adding UEClusterizeApp related information and forwarding to the VirtualisationManager
        void handleClusterizeConfig(ClusterizeConfigPacket *);
};

#endif
