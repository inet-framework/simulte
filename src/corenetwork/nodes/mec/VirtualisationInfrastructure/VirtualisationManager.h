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


#ifndef __SIMULTE_VIRTUALISATIONMANAGER_H_
#define __SIMULTE_VIRTUALISATIONMANAGER_H_

#include <omnetpp.h>

#include "common/LteCommon.h"
#include "corenetwork/binder/LteBinder.h"           //to handle Car dynamically leaving the Network

#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

// ME App Modules to instantiate & connect through gates
#include "apps/vehicular/mec/clusterize/MEClusterizeApp.h"

// UE/ME Packets & UE/ME App to Handle
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/vehicular/mec/clusterize/packets/ClusterizePacketTypes.h"


struct meAppMapEntry{
    int meAppGateIndex;
    cModule* meAppModule;
    inet::L3Address ueAddress;
};

/**
 * VirtualisationManager
 *
 *  The task of this class is:
 *       1) handling all types of ClusterizePacket:
 *              a) START_CLUSTERIZE --> query the ResourceManager & instantiate dinamically a MEClusterizeApp
 *              b) STOP_CLUSTERIZE --> terminate a MEClusterizeApp & query the ResourceManager to free resources
 *              c) replying with ACK_START_CLUSTERIZE or ACK_STOP_CLUSTERIZE
 *              d) forwarding upstream INFO_CLUSTERIZE
 *              e) forwarding downstream CONFIG_CLUSTERIZE
 */

class VirtualisationManager : public cSimpleModule
{
    // reference to the LTE Binder module
    LteBinder* binder_;

    inet::UDPSocket socket;

    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;

    cModule* virtualisationInfr;
    cModule* meHost;
    cModule* mePlatform;

    //needed to configure the ME Application dynamically created
    const char* interfaceTableModule;

    int maxMEApps;
    int currentMEApps;

    //UEClusterizeApp mapping with correspondent MEClusterizeApp
    //
    //key = ueModuleName || v2vAppName --> i.e. key = car[0]V2vAlertSender
    //
    //value = meApp gate index + MEApp cModule + UEApp L3Address
    //
    std::map<std::string, meAppMapEntry> meAppMapTable;

    cModule* meClusterizeService;

    //set of free gates
    std::vector<int> freeGates;

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);

        /*
         * -------------------Packet from ResourceManager handler------------------
         */
        // handling ResourceManager resource allocations
        // receiving back from ResourceManager the start (resource allocated) or stop (resource deallocated) packets
        // calling:
        //          1) instantiateMEClusetrizeAPP() or
        //          2) ackClusterize() with ACK_STOP_CLUSTERIZE
        void handleResource(cMessage*);

        /*
         * --------------------ClusterizePacket handlers---------------------------
         */
        // handling all possible ClusterizePacket types by invoking specific methods
        //
        void handleClusterize(ClusterizePacket*);

        // handling START_CLUSTERIZE ClusterizePacket
        // by forwarding the packet to te ResourceManager if there are available MEApp "free slots"
        void startClusterize(ClusterizePacket*);

        // handling INFO_CLUSTERIZE ClusterizeInfoPacket
        // by forwading upstream to the MEClusterizeApp
        void upstreamClusterize(ClusterizeInfoPacket*);

        // handling CONFIG_CLUSTERIZE ClusterizeConfigPacket
        // by forwarding downstream to the UDP Layer (sending via socket to the UEClusterizeApp)
        void downstreamClusterize(ClusterizeConfigPacket*);

        // handling STOP_CLUSTERIZE ClusterizePacket
        // forwarding the packet to the ResourceManager
        void stopClusterize(ClusterizePacket*);

        // instancing the requested MEClusterizeApp (called by handleResource)
        void instantiateMEClusterizeApp(ClusterizePacket*);

        // terminating the correspondent MEClusterizeApp (called by handleResource)
        //
        void terminateMEClusterizeApp(ClusterizePacket*);

        // sending ACK_START_CLUSTERIZE or ACK_STOP_CLUSTERIZE (called by instantiateMEClusterizeApp or terminateMEClusterizeApp)
        //
        void ackClusterize(ClusterizePacket*, const char*);

};

#endif
