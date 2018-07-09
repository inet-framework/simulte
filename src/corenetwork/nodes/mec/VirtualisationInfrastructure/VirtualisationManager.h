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

//SIMULTE BINDER (Oracle Module) and UTILITIES
#include "common/LteCommon.h"
#include "corenetwork/binder/LteBinder.h"           //to handle Car dynamically leaving the Network

//UDP SOCKET for INET COMMUNICATION WITH UE APPs
#include "inet/transportlayer/contract/udp/UDPSocket.h"
#include "inet/networklayer/common/L3Address.h"
#include "inet/networklayer/common/L3AddressResolver.h"

//MEAppPacket
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_Types.h"
#include "corenetwork/nodes/mec/MEPlatform/MEAppPacket_m.h"

//###########################################################################
//data structures and values

#define NO_SERVICE -1
#define SERVICE_NOT_AVAILABLE -2

struct meAppMapEntry
{
    int meAppGateIndex;         //map key
    cModule* meAppModule;       //for ME App termination
    inet::L3Address ueAddress;  //for downstream using UDP Socket
};
//###########################################################################

/**
 * VirtualisationManager
 *
 *  The task of this class is:
 *       1) handling all types of MEAppPacket:
 *              a) START_MEAPP --> query the ResourceManager & instantiate dynamically the MEApp specified into the meAppModuleType and meAppModuleName fields of the packet
 *              b) STOP_MEAP --> terminate a MEApp & query the ResourceManager to free resources
 *              c) replying with ACK_START_MEAPP or ACK_STOP_MEAPP to the UEApp
 *              d) forwarding upstream INFO_UEAPP packets
 *              e) forwarding downstream INFO_MEAPP packets
 */

class VirtualisationManager : public cSimpleModule
{
    //------------------------------------
    //SIMULTE Binder module
    LteBinder* binder_;
    //------------------------------------
    //communication with UE APPs
    inet::UDPSocket socket;
    int localPort_;
    int destPort_;
    inet::L3Address destAddress_;
    //------------------------------------
    //parent modules
    cModule* meHost;
    cModule* mePlatform;
    cModule* virtualisationInfr;
    //------------------------------------
    const char* interfaceTableModule;
    //------------------------------------
    //parameters to control the number of ME APPs instantiated and to set gate sizes
    int maxMEApps;
    int currentMEApps;
    //------------------------------------
    //set of ME Services loaded into the ME Host & Platform
    int numServices;
    std::vector<cModule*> meServices;
    //------------------------------------
    //set of free gates to use for connecting ME Apps and ME Services
    std::vector<int> freeGates;
    //------------------------------------
    //mapping UEApp with the corresponding MEApp instance (using the meAppIn and meAppOut gate index)
    //key = UE App ID - value = ME App gate index
    std::map<int, int> ueAppIdToMeAppGateIndex;
    //storing the UEApp and MEApp informations
    //key = ME App gate index - value meAppMapEntry
    std::map<int, meAppMapEntry> meAppMap;

    public:
        VirtualisationManager();

    protected:

        virtual int numInitStages() const { return inet::NUM_INIT_STAGES; }
        void initialize(int stage);
        virtual void handleMessage(cMessage *msg);
        /*
         * ----------------------------------Packet from ResourceManager handler---------------------------------
         */
        // receiving back from ResourceManager the start (1. resource allocated) or stop (2. resource deallocated) packets
        // calling:
        //          1) instantiateMEAPP() and then ackMEAppPacket() with ACK_START_MEAPP
        //          2) terminateMEAPP() and then ackMEAppPacket() with ACK_STOP_MEAPP
        void handleResource(cMessage*);
        /*
         * ------------------------------------------------------------------------------------------------------
         */
        /*
         * ------------------------------------------MEAppPacket handlers----------------------------------------
         */
        // handling all possible MEAppPacket types by invoking specific methods
        void handleMEAppPacket(MEAppPacket*);

            // handling START_MEAPP type
            // by forwarding the packet to the ResourceManager if there are available MEApp "free slots"
            void startMEApp(MEAppPacket*);

                //finding the ME Service requested by UE App among the ME Services available on the ME Host
                //return: the index of service (in mePlatform.udpService) or SERVICE_NOT_AVAILABLE or NO_SERVICE
                int findService(const char* serviceName);

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
};

#endif
