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


// UE/ME Packets & UE/ME App to Handle
#include "apps/mec/clusterize/packets/ClusterizePacket_m.h"
#include "apps/mec/clusterize/packets/ClusterizePacketTypes.h"
#include "apps/mec/clusterize/packets/ClusterizePacketBuilder.h"


#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_m.h"

#define NO_SERVICE -1
#define SERVICE_NOT_AVAILABLE -2

struct meAppMapEntry{
    int meAppGateIndex;
    cModule* meAppModule;
    inet::L3Address ueAddress;
};

/**
 * VirtualisationManager
 *
 *  The task of this class is:
 *       1) handling all types of MEAppPacket:
 *              a) START_MEAPP --> query the ResourceManager & instantiate dinamically the MEApp specified into the meAppModuleType and meAppModuleName fields of the packet
 *              b) STOP_MEAP --> terminate a MEApp & query the ResourceManager to free resources
 *              c) replying with ACK_START_MEAPP or ACK_STOP_MEAPP to the UEApp
 *              d) forwarding upstream INFO_UEAPP packets
 *              e) forwarding downstream INFO_MEAPP packets
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
    //key = sourceAddress || MEModuleName  (retrieved from MEAppPacket) --> i.e. key = car[0]MEClusterizeApp

    std::map<std::string, meAppMapEntry> meAppMapTable;

    //set of ME Services loaded into the ME Host & Platform
    int numServices;
    std::vector<cModule*> meServices;

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
        // receiving back from ResourceManager the start (1. resource allocated) or stop (2. resource deallocated) packets
        // calling:
        //          1) instantiateMEAPP() and then ackMEAppPacket() with ACK_START_MEAPP
        //          2) terminateMEAPP() and then ackMEAppPacket() with ACK_STOP_MEAPP
        void handleResource(cMessage*);

        /*
         * --------------------MEAppPacket handlers---------------------------
         */
        // handling all possible MEAppPacket types by invoking specific methods
        //
        void handleMEAppPacket(MEAppPacket*);

        // handling START_MEAPP type
        // by forwarding the packet to the ResourceManager if there are available MEApp "free slots"
        void startMEApp(MEAppPacket*);

        // handling INFO_UEAPP type
        // by forwarding upstream to the MEApp
        void upstreamToMEApp(MEAppPacket*);

        // handling INFO_MEAPP type
        // by forwarding downstream to the UDP Layer (sending via socket to the UEApp)
        void downstreamToUEApp(MEAppPacket*);

        // handling STOP_MEAPP type
        // forwarding the packet to the ResourceManager
        void stopMEApp(MEAppPacket*);

        // instancing the requested MEApp (called by handleResource)
        void instantiateMEApp(MEAppPacket*);

        // terminating the correspondent MEApp (called by handleResource)
        void terminateMEApp(MEAppPacket*);

        // sending ACK_START_MEAPP or ACK_STOP_MEAPP (called by instantiateMEApp or terminateMEApp)
        void ackMEAppPacket(MEAppPacket*, const char*);

        //finding the ME Service requested by UE App among the ME Services available on the ME Host
        //  -1 NO_SERVICE required | -2 SERVICE_NOT_AVAILABLE | index of service in mePlatform.udpService
        int findService(const char* serviceName);
};

#endif
